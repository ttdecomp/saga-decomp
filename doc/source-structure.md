# Source-tree structure

> Agent/reference document. Human development workflows are in
> [`CONTRIBUTING.md`](../CONTRIBUTING.md).

This document explains how the reconstructed source tree relates to the
original Android x86 binary. It is the restructuring reference, but generated
artifacts—not copied counts in prose—are the source of truth.

Snapshot checked on 2026-09-13:

| item | current value |
|---|---:|
| target source translation units | 518 |
| target C translation units | 13 |
| target C++ translation units | 505 |
| generated matching-report units | 518 |
| optimization levels | 172 default `-O0`, 2 `-O1`, 112 `-O2`, 232 `-O3` |
| special compile mode | one of the 232 `-O3` TUs also uses `-fPIE` |
| original `_GLOBAL__sub_I_` symbols | 325 occurrences, 320 unique basenames |

These counts describe the current generated files and will change as the tree
is reorganized. Recompute them with the recipes below rather than updating
code or build configuration merely to make the table agree.

## Ground truth and its limits

The original `res/libTTapp.so` has no DWARF and no `STT_FILE` symbols. Its
compiler-generated `_GLOBAL__sub_I_<basename>` symbols expose 320 unique source
basenames, but only for translation units that required file-scope dynamic
initialization. They are therefore evidence of original filenames and a lower
bound on the original TU count—not a complete TU manifest.

Constant initialization does not create a guard or static-initialization
function. For example, `static int value = 0;` normally occupies `.bss` and
needs no runtime initialization. Do not infer a TU boundary from ordinary
zero-initialized data.

```bash
nm res/libTTapp.so \
  | rg '_GLOBAL__sub_I_' \
  | sed 's/.*_GLOBAL__sub_I_//' \
  | sort -u
```

Do not reference a contributor's `/tmp` file as a canonical list. If a durable
original-basename inventory becomes necessary, generate and commit it from the
command above.

## Current source hierarchy

The current layout is domain-oriented. Important roots are:

| directory | purpose |
|---|---|
| `src/editor/` | editor-specific implementations |
| `src/gameapi/` | AI, editor tools, and GUI APIs |
| `src/gameframework/` | framework services such as save/load |
| `src/gamelib/` | utilities, CRC, terrain, networking, and wind code |
| `src/legoapi/actions/` | game action implementations |
| `src/legoapi/ai/` | game AI and AI-system glue |
| `src/legoapi/audio/` | game audio and embedded Ogg/Vorbis-facing code |
| `src/legoapi/characters/` | character, player, and creature systems |
| `src/legoapi/core/` | configuration, input, game state, and common systems |
| `src/legoapi/cutscenes/` | cutscene systems |
| `src/legoapi/gizmo/` | gizmo base, object, and generated wrapper TUs |
| `src/legoapi/gizmos/` | concrete world-gizmo implementations |
| `src/legoapi/items/`, `menus/`, `props/`, `render/`, `world/` | corresponding gameplay domains |
| `src/legogame/` | startup and platform entry points |
| `src/MechInputTouch/` | Android touch-control system |
| `src/nu2api/nu3d/` | renderer, with `android/` and `generic/` subtrees |
| `src/nu2api/nucore/` | core runtime and Android platform adapters |
| `src/nu2api/nufile/`, `numath/`, `numusic/`, `nusound/` | file, math, music, and sound subsystems |
| `src/host/harness/` | host-only command runner and diagnostic commands |
| `src/host/platform/` | minimal host implementations of external/platform APIs |

Use `rg --files src` for the live tree. Historical flat paths such as
`src/legoapi/qrand.cpp` or `src/legoapi/world.cpp` are no longer authoritative;
their current equivalents include `src/legoapi/core/input/qrand.cpp` and
`src/legoapi/world/world.cpp`.

## Per-file optimization is part of the ABI contract

Optimization is assigned per source file in
`bazel/android_per_file_copts.bazelrc`. A Bazel action query is the final
authority for an existing build.

Moving a function between files can change its optimization level and code
generation even when its source is unchanged. Before moving code, compare the
source and destination commands.

```bash
# Show compile actions and their effective arguments.
bazel aquery --config=target \
  'mnemonic("CppCompile", deps(//src:saga_target))' --output=commands

# Review and validate declared overrides.
rg -n -- '--per_file_copt' bazel/android_per_file_copts.bazelrc
bazel test //scripts/checks:check_bazel_optimization_map
```

Absence of an `-O` flag means `-O0`. `src/legoapi/core/config/cheat.cpp` is
currently the sole `-O3 -fPIE` file.

## Symbol-to-TU lookup

The target is compared as a complete shared object. For one symbol, use:

```bash
bazel run //scripts:objdiff_cli -- _Z5qrandv
```

To find its source owner, search declarations/definitions first, then inspect
the Bazel compile action if the basename is ambiguous:

```bash
rg -n 'qrand' src
bazel aquery --config=target \
  'mnemonic("CppCompile", deps(//src:saga_target))' --output=commands
```

The action query is authoritative for source membership, effective options,
and the object emitted for each source file.

## File-role conventions

- `*_plain.cpp` contains collections of plain C-linkage symbols. This is a
  convention, not a guarantee that every body is an empty stub.
- `*_misc.cpp` is a temporary catch-all. Move a symbol to a better owner only
  when the target path and optimization level are understood.
- `*_types.h` contains provisional type scaffolding. Replace placeholders with
  canonical definitions carefully to avoid ODR and layout errors.
- `android/` contains target platform implementations. The host build keeps
  portable reconstructed Android TUs and selects only true platform seams from
  `host/platform/`; `host/harness/` is never consumed by engine code.
- A shared basename in different directories is not evidence that the files
  should be merged. Directory identity is preserved in current split units.

## Restructuring checklist

1. Resolve the symbol's current source file with `rg` and, if needed, Bazel
   `aquery`.
2. Record the current source and destination compile commands, especially
   `-O2`, `-O3`, and `-fPIE`.
3. Preserve exported names, linkage, signatures, and function definition order.
4. Check for same-named local symbols before moving a static definition.
5. Rebuild the target, run the repository checks, and compare the affected
   symbol directly against `res/libTTapp.so`.

Do not maintain a hand-written 320-row TU map in this document. The current
source list and compile actions come directly from Bazel. A copied table becomes
misleading as soon as the domain hierarchy changes.
