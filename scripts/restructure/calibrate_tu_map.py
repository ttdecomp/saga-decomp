#!/usr/bin/env python3
"""Calibrate conservative TU heuristics against a known current build.

The linked ELF has no reliable source-file records for the original binary.
This tool therefore measures the heuristics on the current build, where each
compile object provides ground-truth ownership.  ``STT_FILE`` records are
removed before inference so the result models the original binary's surface.
No source or build files are changed.
"""

from __future__ import annotations

import argparse
from collections import Counter, defaultdict
import json
from pathlib import Path
import re

from scripts.restructure.elf32 import SHF_ALLOC, read_elf32, workspace_root
from scripts.restructure.inputs import read_units_manifest

STT_FILE = 4
CONSTRUCTOR = re.compile(r"^_GLOBAL__sub_I_(.+)$")


def hide_file_symbols(symbols: list[dict]) -> list[dict]:
    """Return symbols available to inference when ``STT_FILE`` is hidden."""
    return [symbol for symbol in symbols if symbol["type"] != STT_FILE]


def allocated_symbols(sections: list[dict], symbols: list[dict]) -> list[dict]:
    """Keep named, defined, allocated symbols except section/file markers."""
    return [
        symbol
        for symbol in hide_file_symbols(symbols)
        if symbol["type"] not in {3}  # STT_SECTION
        and symbol["section_index"] < len(sections)
        and sections[symbol["section_index"]]["flags"] & SHF_ALLOC
    ]


def _binding_class(symbol: dict) -> int:
    return 0 if symbol["binding"] == 0 else 1


def object_owner_candidates(symbol: dict, units: list[dict]) -> list[int]:
    """Find object owners by exact name, type, and local/non-local class."""
    index = _owner_index(units)
    return sorted(index.get(_owner_key(symbol), ()))


def _owner_key(symbol: dict) -> tuple[str, int, int]:
    return (symbol["name"], symbol["type"], _binding_class(symbol))


def _owner_index(units: list[dict]) -> dict[tuple[str, int, int], set[int]]:
    index: dict[tuple[str, int, int], set[int]] = defaultdict(set)
    for unit_id, unit in enumerate(units):
        for symbol in unit["symbols"]:
            index[_owner_key(symbol)].add(unit_id)
    return index


def local_blocks(symbols: list[dict]) -> tuple[list[dict], dict[int, int]]:
    """Infer symtab-order local blocks, ending at initializer sentinels."""
    local = [symbol for symbol in symbols if symbol["binding"] == 0]
    blocks = []
    assignments = {}
    start = 0
    for offset, symbol in enumerate(local):
        match = CONSTRUCTOR.match(symbol["name"])
        if match is None:
            continue
        members = local[start : offset + 1]
        block_id = len(blocks)
        blocks.append(
            {
                "id": block_id,
                "initializer": symbol["name"],
                "basename": match.group(1),
                "symbol_count": len(members),
            }
        )
        for member in members:
            assignments[member["symbol_index"]] = block_id
        start = offset + 1
    if start < len(local):
        members = local[start:]
        block_id = len(blocks)
        blocks.append(
            {
                "id": block_id,
                "initializer": None,
                "basename": None,
                "symbol_count": len(members),
            }
        )
        for member in members:
            assignments[member["symbol_index"]] = block_id
    return blocks, assignments


def _runs(values: list[int]) -> list[int]:
    if not values:
        return []
    lengths = []
    previous = values[0]
    length = 1
    for value in values[1:]:
        if value == previous:
            length += 1
        else:
            lengths.append(length)
            previous, length = value, 1
    lengths.append(length)
    return lengths


def address_contiguity(symbols: list[dict]) -> dict:
    """Measure text-function runs for symbols with one current object owner."""
    owned = [
        symbol
        for symbol in symbols
        if symbol["type"] == 2
        and symbol.get("section", ".text") == ".text"
        and len(symbol.get("owners", ())) == 1
    ]
    owned.sort(key=lambda symbol: (symbol["address"], symbol["symbol_index"]))
    sequence = [symbol["owners"][0] for symbol in owned]
    lengths = _runs(sequence)
    by_owner: dict[int, list[int]] = defaultdict(list)
    cursor = 0
    for length in lengths:
        by_owner[sequence[cursor]].append(length)
        cursor += length
    substantial = [runs for runs in by_owner.values() if sum(runs) >= 5]
    return {
        "symbols_with_unique_owner": len(owned),
        "runs": len(lengths),
        "largest_run": max(lengths, default=0),
        "largest_run_fraction": (max(lengths) / len(owned)) if owned else 0.0,
        "owners_with_at_least_five_functions": len(substantial),
        "owners_in_one_run": sum(len(runs) == 1 for runs in substantial),
        "fraction_in_owner_largest_run": (
            sum(max(runs) for runs in substantial) / sum(sum(runs) for runs in substantial)
            if substantial
            else 0.0
        ),
    }


def block_purity(symbols: list[dict], assignments: dict[int, int]) -> dict:
    """Measure owner purity and coverage within inferred local blocks."""
    by_block: dict[int, Counter] = defaultdict(Counter)
    for symbol in symbols:
        block = assignments.get(symbol["symbol_index"])
        owners = symbol.get("owners", ())
        if block is not None and len(owners) == 1:
            by_block[block][owners[0]] += 1
    total = sum(sum(counts.values()) for counts in by_block.values())
    pure = 0
    details = []
    for block, counts in sorted(by_block.items()):
        count = sum(counts.values())
        best = max(counts.values(), default=0)
        pure += best
        details.append(
            {
                "id": block,
                "owned_symbols": count,
                "owners": dict(sorted(counts.items())),
                "majority_fraction": best / count if count else 0.0,
            }
        )
    return {
        "blocks_with_unique_owner_symbols": len(by_block),
        "symbols_with_unique_owner": total,
        "majority_owner_fraction": pure / total if total else 0.0,
        "blocks": details,
    }


def preceding_file_labels(symbols: list[dict]) -> dict[int, str]:
    """Associate each local symbol with the preceding ``STT_FILE`` record."""
    labels = {}
    current = None
    for symbol in sorted(symbols, key=lambda item: item["symbol_index"]):
        if symbol["type"] == STT_FILE:
            # Basenames repeat. The FILE record's symtab index is the identity.
            current = f'{symbol["symbol_index"]}:{symbol["name"]}'
        elif symbol["binding"] == 0 and current is not None:
            labels[symbol["symbol_index"]] = current
    return labels


def file_block_purity(symbols: list[dict], assignments: dict[int, int], labels: dict[int, str]) -> dict:
    """Measure whether inferred blocks agree with the ELF's file markers."""
    by_block: dict[int, Counter] = defaultdict(Counter)
    for symbol in symbols:
        block = assignments.get(symbol["symbol_index"])
        label = labels.get(symbol["symbol_index"])
        if block is not None and label is not None:
            by_block[block][label] += 1
    total = sum(sum(counts.values()) for counts in by_block.values())
    majority = sum(max(counts.values(), default=0) for counts in by_block.values())
    return {
        "local_symbols_with_preceding_file": total,
        "blocks_with_file_symbols": len(by_block),
        "mixed_blocks": sum(len(counts) > 1 for counts in by_block.values()),
        "majority_file_fraction": majority / total if total else 0.0,
        "blocks": [
            {
                "id": block,
                "symbols": sum(counts.values()),
                "files": dict(sorted(counts.items())),
            }
            for block, counts in sorted(by_block.items())
        ],
    }


def calibrate(
    linked_symbols: list[dict], units: list[dict], file_symbols: list[dict] | None = None
) -> dict:
    """Return calibration metrics for a parsed linked ELF and object records."""
    # Work on copies: callers may reuse the parser's inventory for another
    # analysis.  ``file_symbols`` is supplied separately because allocated
    # symbol filtering normally removes STT_FILE records.
    symbols = [dict(symbol) for symbol in hide_file_symbols(linked_symbols)]
    files = [dict(symbol) for symbol in (file_symbols or [])]
    labels = preceding_file_labels([*symbols, *files])
    owners = _owner_index(units)
    for symbol in symbols:
        symbol["owners"] = sorted(owners.get(_owner_key(symbol), ()))
    blocks, assignments = local_blocks(symbols)
    return {
        "rules": {
            "file_symbols": "STT_FILE removed before all inference",
            "owner": "exact name/type and local-vs-nonlocal binding class in object symbols",
            "address": "address-sorted sequence of uniquely owned symbols",
            "blocks": "local symtab order ending at _GLOBAL__sub_I_; not claimed as TUs",
        },
        "summary": {
            "linked_symbols_after_file_hiding": len(symbols),
            "file_symbols_hidden": len(files)
            + sum(symbol["type"] == STT_FILE for symbol in linked_symbols),
            "file_symbols_seen": len(files)
            + sum(symbol["type"] == STT_FILE for symbol in linked_symbols),
            "initializer_blocks": len(blocks),
            "unique_owner_symbols": sum(len(s["owners"]) == 1 for s in symbols),
            "ambiguous_owner_symbols": sum(len(s["owners"]) > 1 for s in symbols),
            "unowned_symbols": sum(not s["owners"] for s in symbols),
        },
        "address_contiguity": address_contiguity(symbols),
        "block_purity": block_purity(symbols, assignments),
        "file_ground_truth": file_block_purity(symbols, assignments, labels),
    }


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--current", type=Path, required=True)
    parser.add_argument("--units", type=Path, required=True, help="JSON source/object manifest")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    root = workspace_root()
    current = args.current.resolve()
    units = read_units_manifest(args.units, root)
    sections, symbols = read_elf32(current.resolve(), include_file_symbols=True)
    linked = [
        {**symbol, "section": sections[symbol["section_index"]]["name"]}
        for symbol in allocated_symbols(sections, symbols)
    ]
    files = [symbol for symbol in symbols if symbol["type"] == STT_FILE]
    unit_records = []
    for unit in units:
        object_sections, object_symbols = read_elf32(unit["object_path"])
        unit_records.append({**unit, "symbols": allocated_symbols(object_sections, object_symbols)})
    report = calibrate(linked, unit_records, files)
    output = args.output or root / ".work/tu-map-calibration.json"
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(report["summary"], indent=2))
    print(json.dumps(report["address_contiguity"], indent=2))
    print(json.dumps({k: v for k, v in report["block_purity"].items() if k != "blocks"}, indent=2))
    print(json.dumps({k: v for k, v in report["file_ground_truth"].items() if k != "blocks"}, indent=2))
    print(f"Wrote {output}")


if __name__ == "__main__":
    main()
