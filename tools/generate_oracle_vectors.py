#!/usr/bin/env python3
"""Generate deterministic Bitcoin double-SHA-256 vectors for firmware KATs."""

from __future__ import annotations

import hashlib
from pathlib import Path
import sys


VECTOR_COUNT = 4096
SEED = 0x6D5A56E9


def xorshift32(state: int) -> int:
    state ^= (state << 13) & 0xFFFFFFFF
    state ^= state >> 17
    state ^= (state << 5) & 0xFFFFFFFF
    return state & 0xFFFFFFFF


def main() -> int:
    if len(sys.argv) != 2:
        raise SystemExit("usage: generate_oracle_vectors.py OUTPUT")

    output = Path(sys.argv[1])
    state = SEED
    expected: list[bytes] = []
    fixture_hasher = hashlib.sha256()
    for _ in range(VECTOR_COUNT):
        header = bytearray()
        for _ in range(20):
            state = xorshift32(state)
            header.extend(state.to_bytes(4, "little"))
        state = xorshift32(state)
        nonce = state
        header[76:80] = nonce.to_bytes(4, "little")
        digest = hashlib.sha256(hashlib.sha256(header).digest()).digest()
        expected.append(digest)
        fixture_hasher.update(header)
        fixture_hasher.update(digest)

    lines = [
        "/* Generated deterministically by tools/generate_oracle_vectors.py. */",
        f"#define ORACLE_VECTOR_COUNT {VECTOR_COUNT}u",
        f"#define ORACLE_VECTOR_SEED UINT32_C(0x{SEED:08x})",
        f'#define ORACLE_FIXTURE_SHA256 "{fixture_hasher.hexdigest()}"',
        "static const uint8_t oracle_expected[ORACLE_VECTOR_COUNT][HASH_BYTES] = {",
    ]
    for digest in expected:
        values = ", ".join(f"0x{byte:02x}" for byte in digest)
        lines.append(f"    {{{values}}},")
    lines.append("};")
    lines.append("")

    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text("\n".join(lines), encoding="ascii")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
