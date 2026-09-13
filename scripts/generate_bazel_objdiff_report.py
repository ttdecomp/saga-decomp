#!/usr/bin/env python3
"""Generate the GitHub Pages matching data from the two complete binaries.

The objdiff score comes from one whole-binary comparison. Bazel's action graph
provides the source-to-object mapping, and each produced object file's symbol
table provides translation-unit ownership. The original ELF supplies stable
addresses and sizes for the address map.
"""

from __future__ import annotations

import argparse
from collections import Counter, defaultdict
import json
import os
from pathlib import Path
import re
import struct
import subprocess
import sys
import tempfile

from scripts.lib.bazel_actions import bazel_units

SHF_ALLOC = 0x2
SHF_EXECINSTR = 0x4
SHF_WRITE = 0x1
SHT_NOBITS = 8
SHT_SYMTAB = 2
STT_FUNC = 2

MATCHING_TABLE_START = "<!-- matching-table-start -->"
MATCHING_TABLE_END = "<!-- matching-table-end -->"
ROOT_LABEL = "(root)"


def workspace_root() -> Path:
    """Return the source workspace both under `bazel run` and direct Python."""
    bazel_workspace = os.environ.get("BUILD_WORKSPACE_DIRECTORY")
    if bazel_workspace:
        return Path(bazel_workspace).resolve()
    return Path(__file__).resolve().parents[1]


def _cstring(data: bytes, offset: int) -> str:
    end = data.find(b"\0", offset)
    if end == -1:
        end = len(data)
    return data[offset:end].decode("utf-8", errors="replace")


def read_elf32(
    path: Path, *, include_file_symbols: bool = False
) -> tuple[list[dict], list[dict]]:
    """Read sections and defined symbols from a 32-bit ELF file.

    ``STT_FILE`` uses a special section index and is normally omitted. The
    calibration tool can request those records to test TU-boundary inference.
    """
    data = path.read_bytes()
    if data[:4] != b"\x7fELF" or data[4] != 1:
        raise ValueError(f"{path}: expected a 32-bit ELF file")
    byte_order = {1: "<", 2: ">"}.get(data[5])
    if byte_order is None:
        raise ValueError(f"{path}: unsupported ELF byte order")

    header = struct.unpack_from(byte_order + "HHIIIIIHHHHHH", data, 16)
    section_offset = header[5]
    section_entry_size = header[10]
    section_count = header[11]
    section_names_index = header[12]
    section_struct = struct.Struct(byte_order + "IIIIIIIIII")
    if section_entry_size < section_struct.size:
        raise ValueError(f"{path}: invalid ELF section-header size")

    sections = []
    for index in range(section_count):
        offset = section_offset + index * section_entry_size
        fields = section_struct.unpack_from(data, offset)
        sections.append(
            {
                "index": index,
                "name_offset": fields[0],
                "type": fields[1],
                "flags": fields[2],
                "address": fields[3],
                "offset": fields[4],
                "size": fields[5],
                "link": fields[6],
                "info": fields[7],
                "alignment": fields[8],
                "entry_size": fields[9],
            }
        )

    names_section = sections[section_names_index]
    names = data[
        names_section["offset"] : names_section["offset"] + names_section["size"]
    ]
    for section in sections:
        section["name"] = _cstring(names, section["name_offset"])

    symbols = []
    symbol_struct = struct.Struct(byte_order + "IIIBBH")
    for symbol_table in (
        section for section in sections if section["type"] == SHT_SYMTAB
    ):
        strings_section = sections[symbol_table["link"]]
        strings = data[
            strings_section["offset"] : strings_section["offset"]
            + strings_section["size"]
        ]
        entry_size = symbol_table["entry_size"] or symbol_struct.size
        for symbol_index, offset in enumerate(
            range(
                symbol_table["offset"],
                symbol_table["offset"] + symbol_table["size"],
                entry_size,
            )
        ):
            fields = symbol_struct.unpack_from(data, offset)
            section_index = fields[5]
            is_file = (fields[3] & 0x0F) == 4
            if not fields[0] or (
                section_index >= len(sections)
                and not (include_file_symbols and is_file)
            ):
                continue
            symbols.append(
                {
                    "name": _cstring(strings, fields[0]),
                    "address": fields[1],
                    "size": fields[2],
                    "type": fields[3] & 0x0F,
                    "binding": fields[3] >> 4,
                    "visibility": fields[4] & 0x03,
                    "symbol_index": symbol_index,
                    "symbol_table": symbol_table["name"],
                    "section_index": section_index,
                }
            )
    return sections, symbols


def bazel_target_output(root: Path, bazel: str, target: str) -> Path:
    """Resolve a target-config output without depending on the bazel-bin link."""
    result = subprocess.run(
        [
            bazel,
            "cquery",
            "--config=target",
            target,
            "--output=files",
            "--noshow_progress",
        ],
        cwd=root,
        text=True,
        capture_output=True,
        check=False,
    )
    if result.returncode:
        sys.stderr.write(result.stderr)
        raise RuntimeError(f"bazel cquery failed for {target}")
    outputs = [root / line for line in result.stdout.splitlines() if line.strip()]
    files = [output for output in outputs if output.is_file()]
    if len(files) != 1:
        raise RuntimeError(
            f"expected one built output for {target}, got {len(files)}; "
            "build //src:saga_target with --config=target first"
        )
    return files[0].resolve()


def whole_binary_report(
    root: Path, objdiff: str, original: Path, current: Path
) -> dict:
    """Run objdiff once against the two complete shared libraries."""
    with tempfile.TemporaryDirectory(prefix="saga-objdiff-") as temporary_dir:
        report_path = Path(temporary_dir) / "report.json"
        command = [
            objdiff,
            "report",
            "generate",
            "-1",
            str(original),
            "-2",
            str(current),
            "-o",
            str(report_path),
            "-f",
            "json",
        ]
        result = subprocess.run(command, cwd=root, check=False)
        if result.returncode:
            raise RuntimeError("objdiff-cli report generation failed")
        with report_path.open(encoding="utf-8") as report_file:
            return json.load(report_file)


def _number(value: object) -> int:
    if isinstance(value, int):
        return value
    return int(str(value), 0)


def _section_kind(section: dict) -> str:
    if section["flags"] & SHF_EXECINSTR:
        return "exec"
    if section["type"] == SHT_NOBITS:
        return "nobits"
    if section["flags"] & SHF_WRITE:
        return "write"
    return "read"


def _badge_color(progress: float) -> str:
    if progress >= 90:
        return "brightgreen"
    if progress >= 70:
        return "green"
    if progress >= 50:
        return "yellow"
    if progress >= 30:
        return "orange"
    return "red"


def _matching_table(report: dict) -> list[str]:
    """Aggregate assigned function scores by source directory."""
    stats = defaultdict(
        lambda: {
            "code_size": 0,
            "weighted_score": 0.0,
            "functions": 0,
            "matched_functions": 0,
        }
    )
    for unit in report["units"]:
        source_parts = Path(unit["source"]).parts
        if not source_parts or source_parts[0] != "src" or len(source_parts) < 2:
            continue
        relative_parts = source_parts[1:]
        top_level = relative_parts[0] if len(relative_parts) > 1 else ROOT_LABEL
        if top_level == "host":
            continue
        keys = [top_level]
        if top_level == "legoapi" and len(relative_parts) > 2:
            keys.append(f"legoapi/{relative_parts[1]}")

        for function in unit["functions"]:
            size = int(function["size"])
            score = float(function["match_percent"] or 0.0)
            for key in keys:
                row = stats[key]
                row["code_size"] += size
                row["weighted_score"] += score * size
                row["functions"] += 1
                if score == 100.0:
                    row["matched_functions"] += 1

    lines = [
        "| Directory | Fuzzy % | Funcs % |",
        "|---|---:|---:|",
    ]
    for key in sorted(stats):
        row = stats[key]
        fuzzy = row["weighted_score"] / row["code_size"] if row["code_size"] else 0.0
        functions = (
            100.0 * row["matched_functions"] / row["functions"]
            if row["functions"]
            else 0.0
        )
        lines.append(f"| `{key}` | {fuzzy:.1f}% | {functions:.1f}% |")
    return lines


def update_readme(path: Path, report: dict) -> None:
    """Replace the marked matching table and overall progress badge."""
    content = path.read_text(encoding="utf-8")
    start = content.find(MATCHING_TABLE_START)
    end = content.find(MATCHING_TABLE_END)
    if start == -1 or end == -1 or end <= start:
        raise ValueError(f"{path}: matching table markers are missing or invalid")

    section = "\n".join(
        [
            MATCHING_TABLE_START,
            "",
            "## Matching progress 📊",
            "",
            "See https://ttdecomp.github.io/saga/",
            "",
            *_matching_table(report),
            "",
            MATCHING_TABLE_END,
        ]
    )
    content = content[:start] + section + content[end + len(MATCHING_TABLE_END) :]

    progress = float(report.get("measures", {}).get("fuzzy_match_percent", 0.0))
    badge = (
        f"https://img.shields.io/badge/matching-{progress:.2f}%25-"
        f"{_badge_color(progress)}"
    )
    content = re.sub(r"https://img\.shields\.io/badge/matching-[^)]*", badge, content)
    path.write_text(content, encoding="utf-8")


def build_custom_report(original: Path, report: dict, units: list[dict]) -> dict:
    """Combine whole-binary scores, original addresses, and Bazel ownership."""
    sections, original_symbols = read_elf32(original)
    text_section = next(
        (section for section in sections if section["name"] == ".text"), None
    )
    if text_section is None:
        raise ValueError(f"{original}: no .text section")

    report_units = report.get("units", [])
    if len(report_units) != 1:
        raise ValueError(
            f"expected one whole-binary objdiff unit, got {len(report_units)}"
        )
    report_functions = report_units[0].get("functions", [])
    details_by_key: dict[tuple[int, str], list[dict]] = defaultdict(list)
    for function in report_functions:
        key = (_number(function.get("address", 0)), function["name"])
        details_by_key[key].append(function)

    symbol_units: dict[str, set[str]] = defaultdict(set)
    public_units = []
    for unit in units:
        _object_sections, object_symbols = read_elf32(unit["object_path"])
        for symbol in object_symbols:
            if symbol["type"] == STT_FUNC:
                symbol_units[symbol["name"]].add(unit["name"])
        public_units.append(
            {
                "name": unit["name"],
                "source": unit["source"],
                "object": unit["object"],
                "functions": [],
            }
        )
    units_by_name = {unit["name"]: unit for unit in public_units}

    original_functions = [
        symbol
        for symbol in original_symbols
        if symbol["type"] == STT_FUNC
        and symbol["section_index"] == text_section["index"]
        and symbol["size"]
    ]
    executable_symbols = [
        symbol
        for symbol in original_symbols
        if sections[symbol["section_index"]]["flags"] & SHF_EXECINSTR
    ]
    original_text_symbols = {
        "local": sorted(
            {symbol["name"] for symbol in executable_symbols if symbol["binding"] == 0}
        ),
        "global": sorted(
            {symbol["name"] for symbol in executable_symbols if symbol["binding"] == 1}
        ),
        "weak": sorted(
            {symbol["name"] for symbol in executable_symbols if symbol["binding"] == 2}
        ),
    }
    original_name_counts = Counter(function["name"] for function in original_functions)
    ambiguous_functions = []
    unassigned_functions = []

    for symbol in sorted(
        original_functions, key=lambda item: (item["address"], item["name"])
    ):
        offset = symbol["address"] - text_section["address"]
        candidates = details_by_key.get((offset, symbol["name"]), [])
        detail = candidates[0] if candidates else None
        metadata = detail.get("metadata", {}) if detail else {}
        # Protobuf JSON omits scalar fields whose value is zero.
        score = detail.get("fuzzy_match_percent", 0.0) if detail is not None else None
        candidate_units = sorted(symbol_units.get(symbol["name"], set()))
        record = {
            "name": symbol["name"],
            "demangled_name": metadata.get("demangled_name", symbol["name"]),
            "address": symbol["address"],
            "offset": offset,
            "size": symbol["size"],
            "match_percent": None if score is None else round(float(score), 6),
        }
        ambiguous = original_name_counts[symbol["name"]] > 1 or len(candidate_units) > 1
        if ambiguous:
            record["candidate_units"] = candidate_units
            ambiguous_functions.append(record)
        elif len(candidate_units) == 1:
            units_by_name[candidate_units[0]]["functions"].append(record)
        else:
            unassigned_functions.append(record)

    for unit in public_units:
        unit["functions"].sort(
            key=lambda function: (function["address"], function["name"])
        )

    allocated_sections = [
        {
            "name": section["name"],
            "address": section["address"],
            "size": section["size"],
            "kind": _section_kind(section),
        }
        for section in sections
        if section["flags"] & SHF_ALLOC and section["size"]
    ]
    assigned_count = sum(len(unit["functions"]) for unit in public_units)
    return {
        "schema_version": 1,
        "measures": report.get("measures", {}),
        "text": {
            "address": text_section["address"],
            "size": text_section["size"],
        },
        "sections": allocated_sections,
        "original_text_symbols": original_text_symbols,
        "units": public_units,
        "ambiguous_functions": ambiguous_functions,
        "unassigned_functions": unassigned_functions,
        "summary": {
            "bazel_units": len(public_units),
            "functions": len(original_functions),
            "assigned_functions": assigned_count,
            "ambiguous_functions": len(ambiguous_functions),
            "unassigned_functions": len(unassigned_functions),
        },
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--original", default="res/libTTapp.so")
    parser.add_argument(
        "--current",
        help="current binary (default: target-config output of //src:libTTapp.so)",
    )
    parser.add_argument("--output", default="matching.json")
    parser.add_argument("--readme", default="README.md")
    parser.add_argument(
        "--no-readme",
        action="store_true",
        help="do not update the marked matching table in README.md",
    )
    parser.add_argument("--target", default="//src:saga_target")
    parser.add_argument("--bazel", default="bazel")
    parser.add_argument("--objdiff", default="objdiff-cli")
    args = parser.parse_args()

    root = workspace_root()
    original = (root / args.original).resolve()
    output = (root / args.output).resolve()
    readme = (root / args.readme).resolve()

    try:
        current = (
            (root / args.current).resolve()
            if args.current
            else bazel_target_output(root, args.bazel, "//src:libTTapp.so")
        )
        for binary in (original, current):
            if not binary.is_file():
                raise ValueError(f"missing binary {binary}")
        report = whole_binary_report(root, args.objdiff, original, current)
        units = bazel_units(root, args.bazel, args.target)
        custom_report = build_custom_report(original, report, units)
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(json.dumps(custom_report, indent=2) + "\n", encoding="utf-8")
        if not args.no_readme:
            update_readme(readme, custom_report)
    except (OSError, ValueError, RuntimeError, json.JSONDecodeError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 1

    summary = custom_report["summary"]
    try:
        output_display = output.relative_to(root)
    except ValueError:
        output_display = output
    readme_message = ""
    if not args.no_readme:
        try:
            readme_display = readme.relative_to(root)
        except ValueError:
            readme_display = readme
        readme_message = f" and updated {readme_display}"
    print(
        f"Wrote {summary['functions']} functions in {summary['bazel_units']} "
        f"Bazel units to {output_display}{readme_message} "
        f"({summary['ambiguous_functions']} ambiguous, "
        f"{summary['unassigned_functions']} unassigned)"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
