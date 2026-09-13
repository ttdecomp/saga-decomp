"""Explicit input manifests for optional current-build comparisons."""

from __future__ import annotations

import json
from pathlib import Path


def read_units_manifest(path: Path, root: Path) -> list[dict]:
    """Read source/object pairs from JSON, without running a build system.

    Accept either a list of units or an object with a ``units`` list, such as
    the existing matching report. Paths are resolved relative to ``root``.
    """
    contents = json.loads(path.read_text(encoding="utf-8"))
    entries = contents["units"] if isinstance(contents, dict) else contents
    if not isinstance(entries, list):
        raise ValueError(f"{path}: expected a JSON list of units")
    units = []
    for index, entry in enumerate(entries):
        source = entry["source"]
        object_name = entry["object"]
        object_path = Path(object_name)
        if not object_path.is_absolute():
            object_path = root / object_path
        if not object_path.is_file():
            raise FileNotFoundError(f"{path}: object {index} is missing: {object_path}")
        units.append(
            {
                "source": source,
                "object": object_name,
                "object_path": object_path.resolve(),
                "optimization": entry.get("optimization"),
            }
        )
    return units
