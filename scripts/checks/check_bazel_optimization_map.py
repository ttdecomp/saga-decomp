#!/usr/bin/env python3
"""Validate the repository's checked-in Android per-file optimization map."""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path


BAZEL_OPTION_RE = re.compile(
    r"^\s*build:target\s+--per_file_copt=(.+)@([^\s]+)\s*$"
)
ALLOWED_OPTIONS = {"-O0", "-O1", "-O2", "-O3", "-fPIE", "-fno-ipa-sra"}


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--bazelrc",
        type=Path,
        default=Path("bazel/android_per_file_copts.bazelrc"),
        help="Bazel rc fragment to validate",
    )
    args = parser.parse_args()

    mappings: dict[str, list[str]] = {}
    errors: list[str] = []
    for line_number, line in enumerate(args.bazelrc.read_text().splitlines(), 1):
        match = BAZEL_OPTION_RE.match(line)
        if not match:
            continue
        source_regex, option = match.groups()
        if not source_regex.startswith("^") or not source_regex.endswith("$"):
            errors.append(f"{args.bazelrc}:{line_number}: regex must be anchored")
            continue
        if option not in ALLOWED_OPTIONS:
            errors.append(
                f"{args.bazelrc}:{line_number}: unsupported option {option!r}"
            )
        escaped_source = source_regex[1:-1]
        source = escaped_source.replace(r"\.", ".")
        if re.escape(source) != escaped_source or not source.startswith("src/"):
            errors.append(
                f"{args.bazelrc}:{line_number}: expected an exact escaped src/ path"
            )
            continue
        if not Path(source).is_file():
            errors.append(f"{args.bazelrc}:{line_number}: missing source {source}")
        mappings.setdefault(source, []).append(option)

    if not mappings:
        errors.append(f"{args.bazelrc}: no Android optimization mappings found")

    for source, options in mappings.items():
        optimization_options = [option for option in options if option.startswith("-O")]
        if len(optimization_options) != 1:
            errors.append(
                f"{source}: expected exactly one optimization level, got {options!r}"
            )

    if errors:
        print("Bazel Android optimization map: FAILED", file=sys.stderr)
        print("\n".join(errors), file=sys.stderr)
        return 1

    option_count = sum(len(options) for options in mappings.values())
    print(
        "Bazel Android optimization map: OK "
        f"({len(mappings)} sources, {option_count} options)"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
