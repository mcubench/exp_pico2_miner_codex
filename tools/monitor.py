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


EXPECTED_KATS = {
    "nist_empty",
    "nist_abc",
    "sha_error_sticky",
    "optimized_oracle",
    "target_boundaries",
    "mining_decision_paths",
    "bitcoin_genesis",
    "bitcoin_nonce_search",
}


def fields(line: str) -> dict[str, str]:
    result = {}
    for token in line.split()[1:]:
        if "=" in token:
            key, value = token.split("=", 1)
            result[key] = value
    return result


class ValidationContract:
    def __init__(self, expected_arch: str | None, expected_source: str | None):
        self.expected_arch = expected_arch
        self.expected_source = expected_source
        self.boot = None
        self.kats = set()
        self.summary = False
        self.benchmarks = set()
        self.mining_start = False
        self.progress_count = 0
        self.next_sequence = 1
        self.window_count = 0
        self.next_window = 1

    def observe(self, line: str) -> str | None:
        if line.startswith("TEST:FAIL") or line.startswith("FAULT"):
            return "device reported failure"
        data = fields(line)
        if line.startswith("BOOT "):
            if self.boot is not None:
                return "duplicate BOOT (unexpected reset or stale session)"
            required = {"arch", "source_id", "run_id", "actual_clock_hz"}
            if not required.issubset(data) or not data["run_id"]:
                return "malformed BOOT identity"
            if self.expected_arch and data["arch"] != self.expected_arch:
                return f"wrong architecture {data['arch']}"
            if self.expected_source and data["source_id"] != self.expected_source:
                return f"wrong source identity {data['source_id']}"
            self.boot = data
        elif line.startswith("TEST:PASS "):
            if self.boot is None:
                return "TEST record before BOOT"
            kat = data.get("kat")
            if kat not in EXPECTED_KATS or kat in self.kats:
                return f"unexpected or duplicate KAT {kat}"
            self.kats.add(kat)
            if kat == "mining_decision_paths" and (
                data.get("candidate_nonce") != "2083236893"
                or data.get("candidate_hash")
                != "000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f"
            ):
                return "mining candidate does not match host-known genesis vector"
        elif line.startswith("TEST:SUMMARY "):
            if self.kats != EXPECTED_KATS or data.get("pass") != "8" or data.get("fail") != "0":
                return "summary does not match eight required KATs"
            self.summary = True
        elif line.startswith("BENCHMARK:PASS "):
            if not self.summary:
                return "hardware benchmark before test summary"
            self.benchmarks.add("hardware")
        elif line.startswith("SOFTWARE_BENCHMARK:PASS "):
            if "hardware" not in self.benchmarks:
                return "software benchmark before hardware benchmark"
            self.benchmarks.add("software")
        elif line.startswith("SOFTWARE_FILTER_BENCHMARK:PASS "):
            if "software" not in self.benchmarks:
                return "filter benchmark before full software benchmark"
            self.benchmarks.add("filter")
        elif line.startswith("MINING:START "):
            if self.benchmarks != {"hardware", "software", "filter"}:
                return "MINING:START before required benchmarks"
            if self.boot is None or data.get("run_id") != self.boot["run_id"]:
                return "MINING:START run identity mismatch"
            self.mining_start = True
        elif line.startswith("MINING:PROGRESS "):
            if not self.mining_start or self.boot is None:
                return "MINING:PROGRESS before MINING:START"
            if data.get("run_id") != self.boot["run_id"]:
                return "MINING:PROGRESS run identity mismatch"
            try:
                sequence = int(data.get("sequence", ""))
            except ValueError:
                return "malformed progress sequence"
            if sequence != self.next_sequence:
                return f"progress sequence {sequence}, expected {self.next_sequence}"
            self.next_sequence += 1
            self.progress_count += 1
        elif line.startswith("MEASUREMENT:WINDOW "):
            if self.boot is None or data.get("run_id") != self.boot["run_id"]:
                return "measurement window run identity mismatch"
            try:
                window = int(data.get("window", ""))
                sequence = int(data.get("sequence", ""))
                elapsed = int(data.get("elapsed_us", ""))
                hardware = int(data.get("hardware_hashes", ""))
                software = int(data.get("software_hashes", ""))
                total = int(data.get("total_hashes", ""))
                hardware_rate = int(data.get("hardware_rate_hs", ""))
                software_rate = int(data.get("software_rate_hs", ""))
                total_rate = int(data.get("hash_rate_hs", ""))
            except ValueError:
                return "malformed measurement window"
            if (window != self.next_window or sequence != window * 16
                or sequence != self.next_sequence):
                return "measurement window sequence mismatch"
            if elapsed <= 0 or hardware <= 0 or software <= 0 or total != hardware + software:
                return "invalid measurement window counts"
            rounded = lambda count: (count * 1_000_000 + elapsed // 2) // elapsed
            if (hardware_rate != rounded(hardware)
                or software_rate != rounded(software)
                or total_rate != rounded(total)):
                return "invalid measurement window rate"
            self.next_window += 1
            self.window_count += 1
        return None

    def missing(self) -> list[str]:
        missing = []
        if self.boot is None:
            missing.append("BOOT")
        if self.kats != EXPECTED_KATS or not self.summary:
            missing.append("eight-test summary")
        for stage in ("hardware", "software", "filter"):
            if stage not in self.benchmarks:
                missing.append(f"{stage} benchmark")
        if not self.mining_start:
            missing.append("MINING:START")
        if self.progress_count < 5:
            missing.append(f"five MINING:PROGRESS records ({self.progress_count} seen)")
        if self.window_count < 5:
            missing.append(f"five MEASUREMENT:WINDOW records ({self.window_count} seen)")
        return missing


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
    parser.add_argument("--expected-arch")
    parser.add_argument("--expected-source")
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

    contract = ValidationContract(args.expected_arch, args.expected_source)
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
                error = contract.observe(line)
                if error:
                    print(f"ERROR: validation contract: {error}", file=sys.stderr)
                    return 4
    finally:
        os.close(fd)

    missing = contract.missing()
    if args.require_pass and missing:
        print(
            "ERROR: incomplete validation output: " + ", ".join(missing),
            file=sys.stderr,
        )
        return 5
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
