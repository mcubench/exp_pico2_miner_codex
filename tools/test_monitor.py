#!/usr/bin/env python3
"""Synthetic failure/success coverage for the serial validation contract."""

import importlib.util
from pathlib import Path
import unittest


SPEC = importlib.util.spec_from_file_location(
    "monitor", Path(__file__).with_name("monitor.py")
)
monitor = importlib.util.module_from_spec(SPEC)
assert SPEC.loader is not None
SPEC.loader.exec_module(monitor)


def complete_lines(source="abc123", arch="ARM-M33", run="30004927-00000001"):
    lines = [
        f"BOOT arch={arch} source_id={source} run_id={run} actual_clock_hz=150000000"
    ]
    lines.extend(f"TEST:PASS kat={kat}" for kat in sorted(monitor.EXPECTED_KATS))
    lines.extend(
        [
            "TEST:SUMMARY pass=8 fail=0",
            "BENCHMARK:PASS hash_rate_hs=1",
            "SOFTWARE_BENCHMARK:PASS hash_rate_hs=1",
            "SOFTWARE_FILTER_BENCHMARK:PASS hash_rate_hs=1",
            f"MINING:START run_id={run}",
        ]
    )
    lines.extend(
        f"MINING:PROGRESS run_id={run} sequence={sequence}"
        for sequence in range(1, 6)
    )
    decision = next(
        i for i, line in enumerate(lines)
        if line == "TEST:PASS kat=mining_decision_paths"
    )
    lines[decision] += (
        " candidate_nonce=2083236893"
        " candidate_hash=000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f"
    )
    return lines


class ContractTests(unittest.TestCase):
    def validate(self, lines, source="abc123", arch="ARM-M33"):
        contract = monitor.ValidationContract(arch, source)
        errors = [error for line in lines if (error := contract.observe(line))]
        return contract, errors

    def test_complete_session(self):
        contract, errors = self.validate(complete_lines())
        self.assertEqual(errors, [])
        self.assertEqual(contract.missing(), [])

    def test_missing_stage_is_rejected(self):
        contract, errors = self.validate(complete_lines()[:-1])
        self.assertEqual(errors, [])
        self.assertIn("five MINING:PROGRESS records (4 seen)", contract.missing())

    def test_wrong_source_is_rejected(self):
        _, errors = self.validate(complete_lines(source="wrong"))
        self.assertIn("wrong source identity wrong", errors)

    def test_duplicate_boot_is_rejected(self):
        lines = complete_lines()
        lines.insert(1, lines[0])
        _, errors = self.validate(lines)
        self.assertTrue(any("duplicate BOOT" in error for error in errors))

    def test_sequence_gap_is_rejected(self):
        lines = complete_lines()
        progress = next(i for i, line in enumerate(lines) if line.startswith("MINING:PROGRESS"))
        lines[progress] = lines[progress].replace("sequence=1", "sequence=2")
        _, errors = self.validate(lines)
        self.assertTrue(any("expected 1" in error for error in errors))

    def test_fault_is_rejected(self):
        _, errors = self.validate([*complete_lines(), "FAULT type=test"])
        self.assertIn("device reported failure", errors)

    def test_wrong_candidate_is_rejected(self):
        lines = complete_lines()
        candidate = next(i for i, line in enumerate(lines) if "candidate_hash=" in line)
        lines[candidate] = lines[candidate].replace("candidate_hash=000", "candidate_hash=100")
        _, errors = self.validate(lines)
        self.assertTrue(any("host-known genesis" in error for error in errors))


if __name__ == "__main__":
    unittest.main()
