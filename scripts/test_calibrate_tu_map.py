"""Tests for current-build TU heuristic calibration."""

import unittest

from scripts.calibrate_tu_map import (
    address_contiguity,
    block_purity,
    calibrate,
    hide_file_symbols,
    local_blocks,
    object_owner_candidates,
)


def symbol(index, name, *, address=0, binding=0, type=2):
    return {
        "symbol_index": index,
        "name": name,
        "address": address,
        "binding": binding,
        "type": type,
    }


class CalibrateTuMapTest(unittest.TestCase):
    def test_file_symbols_are_hidden(self):
        self.assertEqual(hide_file_symbols([symbol(1, "x"), symbol(2, "f", type=4)]), [symbol(1, "x")])

    def test_owner_matching_preserves_locality(self):
        units = [{"symbols": [symbol(1, "same")]}, {"symbols": [symbol(2, "same", binding=1)]}]
        self.assertEqual(object_owner_candidates(symbol(4, "same"), units), [0])
        self.assertEqual(object_owner_candidates(symbol(5, "same", binding=1), units), [1])

    def test_block_purity_and_address_runs(self):
        linked = [
            symbol(1, "local_a", address=10),
            symbol(2, "_GLOBAL__sub_I_A.cpp", address=20),
            symbol(3, "local_b", address=30),
            symbol(4, "local_c", address=40),
        ]
        blocks, assignments = local_blocks(linked)
        self.assertEqual(len(blocks), 2)
        for item, owner in zip(linked, [0, 0, 1, 1]):
            item["owners"] = [owner]
        purity = block_purity(linked, assignments)
        self.assertEqual(purity["majority_owner_fraction"], 1.0)
        self.assertEqual(address_contiguity(linked)["largest_run_fraction"], 0.5)

    def test_mixed_initializer_block_is_reported_as_non_pure(self):
        linked = [
            symbol(1, "from_first_tu"),
            symbol(2, "from_second_tu"),
            symbol(3, "_GLOBAL__sub_I_First.cpp"),
        ]
        blocks, assignments = local_blocks(linked)
        for item, owner in zip(linked, [0, 1, 0]):
            item["owners"] = [owner]
        purity = block_purity(linked, assignments)
        self.assertEqual(len(blocks), 1)
        self.assertEqual(purity["symbols_with_unique_owner"], 3)
        self.assertAlmostEqual(purity["majority_owner_fraction"], 2 / 3)

    def test_calibrate_counts_ambiguous_and_unowned(self):
        linked = [symbol(1, "local", address=10), symbol(2, "missing", address=20), symbol(3, "file", type=4)]
        units = [{"symbols": [symbol(8, "local")]}, {"symbols": [symbol(9, "local")]}]
        report = calibrate(linked, units)
        self.assertEqual(report["summary"]["file_symbols_hidden"], 1)
        self.assertEqual(report["summary"]["ambiguous_owner_symbols"], 1)
        self.assertEqual(report["summary"]["unowned_symbols"], 1)

    def test_preceding_file_records_are_ground_truth_for_local_blocks(self):
        linked = [symbol(2, "local_a"), symbol(4, "local_b")]
        files = [symbol(1, "A.cpp", type=4), symbol(3, "A.cpp", type=4)]
        report = calibrate(linked, [], files)
        self.assertEqual(report["summary"]["file_symbols_seen"], 2)
        self.assertEqual(report["file_ground_truth"]["local_symbols_with_preceding_file"], 2)
        self.assertAlmostEqual(report["file_ground_truth"]["majority_file_fraction"], 0.5)


if __name__ == "__main__":
    unittest.main()
