#!/usr/bin/env python3
"""Inventory original allocated symbols and evidence for translation-unit ownership.

This is an evidence ledger, not a claimed recovery of absent STT_FILE records.
Every named, defined, allocated symbol is retained, including aliases and
zero-sized symbols. Current-build comparison is optional and takes explicit
file paths; this tool never invokes a build system.
"""

from __future__ import annotations

import argparse
from collections import Counter, defaultdict
import json
from pathlib import Path
import re
import struct
import subprocess

from scripts.restructure.elf32 import SHF_ALLOC, read_elf32, workspace_root
from scripts.restructure.inputs import read_units_manifest

EXCLUDED_TYPES = {3, 4}  # STT_SECTION, STT_FILE
CONSTRUCTOR = re.compile(r"^_GLOBAL__sub_I_(.+)$")
EMBEDDED_PATH = re.compile(rb"([A-Za-z]:/[A-Za-z0-9_./-]+\.(?:cpp|c))(?::\d+)?\x00")


def allocated_symbols(sections: list[dict], symbols: list[dict]) -> list[dict]:
    """Keep original symbol-table identity, even for aliases at one address."""
    return [
        symbol
        for symbol in symbols
        if sections[symbol["section_index"]]["flags"] & SHF_ALLOC
        and symbol["type"] not in EXCLUDED_TYPES
    ]


def alias_groups(symbols: list[dict]) -> tuple[list[dict], dict[int, int]]:
    """Identify same-location/same-size/type entries without collapsing them."""
    by_location: dict[tuple[int, int, int, int], list[int]] = defaultdict(list)
    for symbol in symbols:
        key = (
            symbol["section_index"],
            symbol["address"],
            symbol["size"],
            symbol["type"],
        )
        by_location[key].append(symbol["symbol_index"])
    groups = []
    assignments = {}
    for key, members in by_location.items():
        if len(members) < 2:
            continue
        group_id = len(groups)
        groups.append({"id": group_id, "location": list(key), "symbol_indices": members})
        for symbol_index in members:
            assignments[symbol_index] = group_id
    return groups, assignments


def local_initializer_blocks(symbols: list[dict]) -> tuple[list[dict], dict[int, int]]:
    """Partition local symtab order at named initializer sentinels.

    A block can contain symbols from initializer-free units. Its basename is
    evidence for its own locals, not a proven owner for the entire block.
    """
    local = [symbol for symbol in symbols if symbol["binding"] == 0]
    blocks = []
    assignments = {}
    start = 0
    for offset, symbol in enumerate(local):
        match = CONSTRUCTOR.match(symbol["name"])
        if match is None:
            continue
        block_id = len(blocks)
        members = local[start : offset + 1]
        blocks.append(
            {
                "id": block_id,
                "initializer": symbol["name"],
                "basename": match.group(1),
                "first_symbol_index": members[0]["symbol_index"],
                "last_symbol_index": symbol["symbol_index"],
                "symbol_count": len(members),
                "certainty": "initializer-delimited; may contain other units",
            }
        )
        for member in members:
            assignments[member["symbol_index"]] = block_id
        start = offset + 1
    if start < len(local):
        block_id = len(blocks)
        members = local[start:]
        blocks.append(
            {
                "id": block_id,
                "initializer": None,
                "basename": None,
                "first_symbol_index": members[0]["symbol_index"],
                "last_symbol_index": members[-1]["symbol_index"],
                "symbol_count": len(members),
                "certainty": "undelimited tail",
            }
        )
        for member in members:
            assignments[member["symbol_index"]] = block_id
    return blocks, assignments


def original_build_clues(
    original: Path, sections: list[dict], symbols: list[dict]
) -> tuple[list[dict], list[str]]:
    """Read constructor order and literal source paths without guessing owners."""
    data = original.read_bytes()
    byte_order = {1: "<", 2: ">"}[data[5]]
    by_address = defaultdict(list)
    for symbol in symbols:
        by_address[symbol["address"]].append(symbol)
    initializers = []
    for section in sections:
        if section["name"] != ".init_array":
            continue
        contents = data[section["offset"] : section["offset"] + section["size"]]
        for ordinal, (address,) in enumerate(struct.iter_unpack(byte_order + "I", contents)):
            initializers.append(
                {
                    "ordinal": ordinal,
                    "address": address,
                    "symbols": [
                        {"name": symbol["name"], "symbol_index": symbol["symbol_index"]}
                        for symbol in by_address[address]
                    ],
                }
            )
    paths = sorted(
        {
            match.group(1).decode("ascii")
            for match in EMBEDDED_PATH.finditer(data)
        }
    )
    return initializers, paths


def function_local_anchors(symbols: list[dict]) -> dict[int, list[int]]:
    """Link Itanium function-local symbols to parent function names, if unique.

    This is a lexical relationship in symbol names. It does not by itself
    assign either symbol to a translation unit.
    """
    relevant = [
        symbol
        for symbol in symbols
        if symbol["type"] == 2 or symbol["name"].startswith("_ZZ")
    ]
    result = subprocess.run(
        ["c++filt"],
        input="\n".join(symbol["name"] for symbol in relevant) + "\n",
        text=True,
        capture_output=True,
        check=True,
    )
    names = result.stdout.splitlines()
    if len(names) != len(relevant):
        raise RuntimeError("c++filt returned an unexpected number of symbol names")
    functions: dict[str, list[int]] = defaultdict(list)
    for symbol, demangled in zip(relevant, names):
        if symbol["type"] == 2:
            functions[demangled].append(symbol["symbol_index"])
    anchors = {}
    for symbol, demangled in zip(relevant, names):
        if symbol["name"].startswith("_ZZ") and "::" in demangled:
            parent = demangled.rsplit("::", 1)[0]
            if parent in functions:
                anchors[symbol["symbol_index"]] = functions[parent]
    return anchors


def build_map(
    original: Path, current: Path | None = None, units: list[dict] | None = None
) -> dict:
    if (current is None) != (units is None):
        raise ValueError("current ELF and unit manifest must be supplied together")
    original_sections, original_all = read_elf32(original)
    original_symbols = allocated_symbols(original_sections, original_all)
    aliases, alias_assignment = alias_groups(original_symbols)
    current_symbols = []
    if current is not None:
        current_sections, current_all = read_elf32(current)
        current_symbols = allocated_symbols(current_sections, current_all)
    blocks, local_blocks = local_initializer_blocks(original_all)
    local_anchors = function_local_anchors(original_symbols)
    initializers, embedded_paths = original_build_clues(
        original, original_sections, original_all
    )

    unit_records = []
    name_owners: dict[tuple[str, int, int], set[int]] = defaultdict(set)
    for unit_id, unit in enumerate(units or []):
        sections, all_symbols = read_elf32(unit["object_path"])
        object_symbols = allocated_symbols(sections, all_symbols)
        entries = []
        for symbol in object_symbols:
            entries.append(
                {
                    **symbol,
                    "section": sections[symbol["section_index"]]["name"],
                }
            )
            # A local symbol never establishes cross-TU ownership by name.
            binding_class = 0 if symbol["binding"] == 0 else 1
            name_owners[(symbol["name"], binding_class, symbol["type"])].add(unit_id)
        unit_records.append(
            {
                "id": unit_id,
                "source": unit["source"],
                "object": unit["object"],
                "optimization": unit["optimization"],
                "symbols": entries,
            }
        )

    mapped = []
    candidate_counts = Counter()
    candidate_by_section: dict[str, Counter] = defaultdict(Counter)
    for symbol in original_symbols:
        entry = {
            **symbol,
            "section": original_sections[symbol["section_index"]]["name"],
            "local_initializer_block": local_blocks.get(symbol["symbol_index"])
            if symbol["binding"] == 0
            else None,
            "alias_group": alias_assignment.get(symbol["symbol_index"]),
            "containing_function_symbols": local_anchors.get(
                symbol["symbol_index"], []
            ),
        }
        if current is not None:
            binding_class = 0 if symbol["binding"] == 0 else 1
            candidates = sorted(
                name_owners.get((symbol["name"], binding_class, symbol["type"]), ())
            )
            if symbol["binding"] == 0:
                evidence = "local-name-only; requires independent corroboration"
            else:
                evidence = "global-name-match; current owner, not original TU proof"
            candidate_class = (
                "unique" if len(candidates) == 1 else "ambiguous" if candidates else "none"
            )
            candidate_counts[candidate_class] += 1
            candidate_by_section[entry["section"]][candidate_class] += 1
            entry["current_owner_candidates"] = candidates
            entry["candidate_evidence"] = (
                evidence if candidates else "no same-name current symbol"
            )
        mapped.append(entry)

    original_section_counts = Counter(symbol["section"] for symbol in mapped)
    summary = {
        "original_allocated_symbols": len(mapped),
        "initializer_delimited_blocks": len(blocks),
        "init_array_entries": len(initializers),
        "embedded_source_paths": len(embedded_paths),
        "function_local_static_anchors": len(local_anchors),
        "alias_groups": len(aliases),
        "original_by_section": dict(sorted(original_section_counts.items())),
    }
    if current is not None:
        summary.update(
            {
                "current_allocated_symbols": len(current_symbols),
                "current_target_units": len(unit_records),
                "current_candidate_counts": dict(sorted(candidate_counts.items())),
                "current_candidates_by_original_section": {
                    section: dict(sorted(counts.items()))
                    for section, counts in sorted(candidate_by_section.items())
                },
            }
        )
    return {
        "schema_version": 1,
        "rules": {
            "inclusion": "named defined SHT_SYMTAB symbols in SHF_ALLOC sections, excluding STT_SECTION and STT_FILE",
            "local_blocks": "symtab-local-order segments ending at _GLOBAL__sub_I_; not necessarily complete TUs",
            "current_candidates": "same-name, same-type object symbols of the same local/nonlocal binding class; not original TU assignment",
            "aliases": "preserved as separate entries by symbol_table and symbol_index",
        },
        "original": str(original),
        "current": str(current) if current is not None else None,
        "summary": summary,
        "original_local_blocks": blocks,
        "original_alias_groups": aliases,
        "original_initializers": initializers,
        "original_embedded_paths": embedded_paths,
        "original_symbols": mapped,
        "current_units": unit_records,
    }


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--original", type=Path)
    parser.add_argument("--current", type=Path)
    parser.add_argument("--units", type=Path, help="JSON source/object manifest")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    if (args.current is None) != (args.units is None):
        parser.error("--current and --units must be supplied together")
    root = workspace_root()
    original = args.original or root / "res/libTTapp.so"
    output = args.output or root / ".work/original-tu-map.json"
    units = read_units_manifest(args.units, root) if args.units else None
    inventory = build_map(
        original.resolve(), args.current.resolve() if args.current else None, units
    )
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(inventory, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(inventory["summary"], indent=2))
    print(f"Wrote {output}")


if __name__ == "__main__":
    main()
