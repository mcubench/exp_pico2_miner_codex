#!/usr/bin/env python3
"""Finite Pico USB-serial capture using only the Python standard library."""

import argparse
import errno
import glob
import os
from pathlib import Path
import select
import sys
import termios
import time
import tty


def is_raspberry_pi_tty(device: str) -> bool:
    try:
        current = Path("/sys/class/tty", Path(device).name).resolve()
    except OSError:
        return False
    for parent in (current, *current.parents):
        vendor = parent / "idVendor"
        try:
            if vendor.read_text(encoding="ascii").strip().lower() == "2e8a":
                return True
        except OSError:
            pass
    return False


def find_port(explicit: str | None) -> str | None:
    if explicit:
        return explicit if os.path.exists(explicit) else None
    env_port = os.environ.get("PICO_PORT")
    if env_port:
        return env_port if os.path.exists(env_port) else None
    ports = [p for p in glob.glob("/dev/ttyACM*") if is_raspberry_pi_tty(p)]
    if len(ports) > 1:
        raise RuntimeError(
            "Multiple Raspberry Pi serial devices found: "
            + ", ".join(ports)
            + ". Set PICO_PORT."
        )
    return ports[0] if ports else None


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--seconds", type=float, default=8.0)
    parser.add_argument("--port")
    parser.add_argument("--require-pass", action="store_true")
    args = parser.parse_args()

    deadline = time.monotonic() + args.seconds
    port = None
    fd = None
    permission_denied = False
    while time.monotonic() < deadline and fd is None:
        port = find_port(args.port)
        if port is None:
            time.sleep(0.2)
            continue
        try:
            fd = os.open(port, os.O_RDONLY | os.O_NOCTTY | os.O_NONBLOCK)
        except PermissionError:
            # A newly enumerated tty can exist briefly before udev applies its
            # final group and ACL. Retry rather than failing the hardware cycle.
            permission_denied = True
            time.sleep(0.2)
        except FileNotFoundError:
            port = None
            time.sleep(0.2)
    if fd is None and permission_denied:
        print(
            f"ERROR: permission denied opening {port}; add the user to dialout and log in again",
            file=sys.stderr,
        )
        return 3
    if fd is None or port is None:
        print("ERROR: Raspberry Pi USB serial device did not appear", file=sys.stderr)
        return 2

    print(f"SERIAL_PORT={port}", flush=True)

    saw_health = False
    pending = b""
    try:
        tty.setraw(fd)
        while time.monotonic() < deadline:
            readable, _, _ = select.select([fd], [], [], min(0.25, deadline - time.monotonic()))
            if not readable:
                continue
            try:
                data = os.read(fd, 4096)
            except OSError as exc:
                if exc.errno in (errno.EAGAIN, errno.EIO):
                    time.sleep(0.1)
                    continue
                raise
            if not data:
                continue
            pending += data
            while b"\n" in pending:
                raw, pending = pending.split(b"\n", 1)
                line = raw.rstrip(b"\r").decode("utf-8", errors="replace")
                print(line, flush=True)
                saw_health = saw_health or line.startswith(("TEST:PASS", "HEARTBEAT"))
                if line.startswith("TEST:FAIL") or line.startswith("FAULT"):
                    return 4
    finally:
        os.close(fd)

    if args.require_pass and not saw_health:
        print("ERROR: no TEST:PASS or HEARTBEAT line observed", file=sys.stderr)
        return 5
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
