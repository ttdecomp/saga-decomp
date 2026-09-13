"""Guard the lossless symbol inventory and conservative TU evidence rules."""

import unittest
from pathlib import Path
import struct
import tempfile

from scripts.generate_original_tu_map import (
    allocated_symbols,
    alias_groups,
    function_local_anchors,
    local_initializer_blocks,
    original_build_clues,
)


class OriginalTuMapTest(unittest.TestCase):
    def test_allocated_symbols_keep_aliases_and_zero_size(self):
        sections = [{"flags": 0}, {"flags": 2}, {"flags": 0}]
        symbols = [
            {"symbol_index": 1, "section_index": 1, "type": 2, "size": 0},
            {"symbol_index": 2, "section_index": 1, "type": 2, "size": 0},
            {"symbol_index": 3, "section_index": 1, "type": 1, "size": 4},
            {"symbol_index": 4, "section_index": 1, "type": 3, "size": 0},
            {"symbol_index": 5, "section_index": 2, "type": 2, "size": 4},
        ]
        self.assertEqual(
            [symbol["symbol_index"] for symbol in allocated_symbols(sections, symbols)],
            [1, 2, 3],
        )

    def test_alias_group_preserves_every_symbol_identity(self):
        base = {"section_index": 1, "address": 0x1234, "size": 4, "type": 1}
        symbols = [
            {**base, "symbol_index": 1},
            {**base, "symbol_index": 2},
            {**base, "symbol_index": 3, "size": 8},
        ]
        groups, assignments = alias_groups(symbols)
        self.assertEqual(groups[0]["symbol_indices"], [1, 2])
        self.assertEqual(assignments, {1: 0, 2: 0})

    def test_initializer_segments_are_not_claimed_as_tus(self):
        symbols = [
            {"symbol_index": 1, "binding": 0, "name": "local_from_another_unit"},
            {"symbol_index": 2, "binding": 0, "name": "_GLOBAL__sub_I_Foo.cpp"},
            {"symbol_index": 3, "binding": 0, "name": "unbounded_local"},
            {"symbol_index": 4, "binding": 1, "name": "global"},
        ]
        blocks, assignments = local_initializer_blocks(symbols)
        self.assertEqual([block["basename"] for block in blocks], ["Foo.cpp", None])
        self.assertEqual(assignments, {1: 0, 2: 0, 3: 1})
        self.assertIn("may contain other units", blocks[0]["certainty"])

    def test_initializers_and_embedded_paths_are_kept_separate(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "binary"
            contents = bytearray(100)
            contents[5] = 1
            contents[16:24] = struct.pack("<II", 0x1000, 0x2000)
            contents[40:58] = b"i:/src/example.cpp\0"
            path.write_bytes(contents)
            sections = [{"name": ".init_array", "offset": 16, "size": 8}]
            symbols = [
                {"address": 0x1000, "name": "_GLOBAL__sub_I_example.cpp", "symbol_index": 1}
            ]
            initializers, paths = original_build_clues(path, sections, symbols)
            self.assertEqual([entry["address"] for entry in initializers], [0x1000, 0x2000])
            self.assertEqual(initializers[0]["symbols"][0]["symbol_index"], 1)
            self.assertEqual(paths, ["i:/src/example.cpp"])

    def test_function_local_static_anchors_parent_function(self):
        symbols = [
            {"symbol_index": 1, "name": "NuIOS_YieldThread", "type": 2},
            {"symbol_index": 2, "name": "_ZZ17NuIOS_YieldThreadE5count", "type": 1},
        ]
        self.assertEqual(function_local_anchors(symbols), {2: [1]})


if __name__ == "__main__":
    unittest.main()
