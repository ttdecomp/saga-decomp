"""Read source-to-object mappings from Bazel's C++ compile actions."""

from pathlib import Path
import shlex
import subprocess
import sys


def _argument_after(arguments: list[str], option: str) -> str | None:
    try:
        return arguments[arguments.index(option) + 1]
    except (ValueError, IndexError):
        return None


def bazel_units(root: Path, bazel: str, target: str) -> list[dict]:
    """Return built source/object pairs for ``target`` C++ compile actions."""
    query = f'mnemonic("CppCompile", deps({target}))'
    result = subprocess.run(
        [
            bazel,
            "aquery",
            "--config=target",
            query,
            "--output=commands",
            "--noshow_progress",
        ],
        cwd=root,
        text=True,
        capture_output=True,
        check=False,
    )
    if result.returncode:
        sys.stderr.write(result.stderr)
        raise RuntimeError("bazel aquery failed")

    source_root = (root / "src").resolve()
    units = []
    seen_sources = set()
    for line in result.stdout.splitlines():
        try:
            arguments = shlex.split(line)
        except ValueError as error:
            raise RuntimeError(f"could not parse Bazel compile action: {error}") from error
        source_arg = _argument_after(arguments, "-c")
        object_arg = _argument_after(arguments, "-o")
        if source_arg is None or object_arg is None:
            continue
        source = Path(source_arg)
        source_abs = source.resolve() if source.is_absolute() else (root / source).resolve()
        try:
            relative_source = source_abs.relative_to(source_root)
        except ValueError:
            continue
        if relative_source in seen_sources:
            raise RuntimeError(f"multiple Bazel compile actions for src/{relative_source}")
        seen_sources.add(relative_source)

        object_path = Path(object_arg)
        object_abs = object_path if object_path.is_absolute() else root / object_path
        if not object_abs.is_file():
            raise RuntimeError(
                f"missing Bazel object for src/{relative_source}: {object_arg}; "
                "build //src:saga_target first"
            )
        units.append(
            {
                "name": f"{relative_source.as_posix()}.o",
                "source": f"src/{relative_source.as_posix()}",
                "object": object_path.as_posix(),
                "object_path": object_abs,
            }
        )
    if not units:
        raise RuntimeError("Bazel query returned no src/ compile actions")
    return sorted(units, key=lambda unit: unit["name"])
