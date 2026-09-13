# 05 — Source conventions

> Agent/reference document. Human source conventions are summarized in
> [`CONTRIBUTING.md`](../../CONTRIBUTING.md).

Guide to placing and authoring reconstructed code in the current `src/` tree.
The tree is actively reorganized, so use live paths and Bazel actions instead
of a copied basename map.

## 1. Source layout

The major roots are:

| directory             | role                                                                                                                                                            |
| --------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `src/editor/`         | editor implementations                                                                                                                                          |
| `src/gameapi/`        | AI, editor-tool, and GUI APIs                                                                                                                                   |
| `src/gameframework/`  | framework systems such as save/load                                                                                                                             |
| `src/gamelib/`        | utilities and supporting engine systems                                                                                                                         |
| `src/legoapi/`        | gameplay, grouped into `actions`, `ai`, `audio`, `characters`, `core`, `cutscenes`, `gizmo`, `gizmos`, `items`, `menus`, `misc`, `props`, `render`, and `world` |
| `src/legogame/`       | startup and platform entry points                                                                                                                               |
| `src/MechInputTouch/` | Android touch controls                                                                                                                                          |
| `src/nu2api/`         | core engine modules (`nu3d`, `nucore`, `nufile`, `numath`, `numusic`, `nuplatform`, `nusound`, and platform glue)                                               |
| `src/java/`           | JNI compatibility code                                                                                                                                          |
| `src/host/harness/`   | host command runner and diagnostic commands                                                                                                                     |
| `src/host/platform/`  | minimal host implementations of external/platform APIs                                                                                                          |

`src/BUILD.bazel` is the source-membership authority. Use `bazel aquery` for the
current set and effective command lines.

The detailed restructuring rules and current commands live in
[`../source-structure.md`](../source-structure.md).

## 2. Resolve ownership; do not guess it

Prefix heuristics are useful for narrowing a search (`NuSound*` generally
belongs under `nu2api/nusound`, `Mech*` under `MechInputTouch`, and so on), but
they are not authoritative. Same basenames can legitimately exist in different
modules.

Preferred one-symbol workflow:

```bash
bazel run //scripts:objdiff_cli -- SYMBOL
```

Locate the source definition and inspect its compile action:

```bash
rg -n 'SYMBOL_OR_DEMANGLED_NAME' src
bazel aquery --config=target \
  'mnemonic("CppCompile", deps(//src:saga_target))' --output=commands
```

If the original symbol has no source definition, classify it as missing game
code, compiler/runtime support, or third-party library code before adding a
stub.

## 3. File-role patterns

- `*_plain.cpp` groups plain C-linkage definitions. Eleven such files currently
  exist; count them with `find src -name '*_plain.cpp'` rather than maintaining
  a list here.
- `*_misc.cpp` is a catch-all for symbols whose better owner is not yet known.
  Treat it as provisional; moving a function still requires matching the
  destination optimization level.
- `*_types.h` contains provisional/Ghidra-derived type scaffolding. Avoid
  duplicating a type already defined canonically elsewhere.
- `android/` contains target platform code. The host build reuses portable
  reconstructed Android TUs; `host/platform/` supplies only true platform
  seams and is not part of the target TU set.
- `*_gen.cpp` denotes manually maintained split/generated content; the suffix
  does not imply a regeneration tool exists.

Examples of current paths:

- qrand: `src/legoapi/core/input/qrand.cpp`
- world: `src/legoapi/world/world.cpp`
- players: `src/legoapi/characters/core/players.cpp`
- AI system: `src/legoapi/ai/core/ai_sys.cpp`
- game audio: `src/legoapi/audio/sfx.cpp`
- gizmo actions: `src/legoapi/gizmo/gizmos/gizmos_gizactions.cpp`
- Ogg-facing code: `src/legoapi/audio/gamelib_ogg.cpp`

## 4. Naming and linkage

Follow `CONTRIBUTING.md`:

- Preserve known original function/type names and every provisioned mangled
  symbol.
- Use descriptive `snake_case` for members, parameters, and locals.
- Use `UPPER_SNAKE_CASE` for enum members and macros.
- C-style tags commonly use lowercase names ending in `_s`/`_e`, with an
  uppercase typedef; C++ classes use `PascalCase`.

Linkage follows the target symbol:

- plain symbol name → normally `extern "C"`
- `_Z...` symbol → C++ linkage

Changing `int` to `long`, signedness, const qualification, a namespace, or a
class name can change mangling even when layout is unchanged. See
[`04-types-abi.md`](04-types-abi.md).

Use `__attribute__` only when it is genuinely necessary for a documented
linkage, layout, platform, or runtime requirement. Verify the need against the
original binary or the relevant build/runtime contract, and explain it near
the declaration. Do not add an attribute merely to improve an objdiff score
or force an assembly shape; recover the underlying types, data, and control
flow instead. In particular, calling-convention attributes are prohibited as
matching shortcuts (see [04-types-abi.md](04-types-abi.md)).

## 5. Types and placeholders

- Use `u8/u16/u32/u64`, `i8/i16/i32/i64`, `f32/f64`, `usize`, and `isize` from
  the engine headers.
- Use `abi_long`/`abi_ulong` only when the target mangle requires `long`/`unsigned long`.
- Match placeholder struct offsets and total size exactly. A field name such as
  `field3_0x14` documents uncertainty; it does not excuse a wrong offset.
- Do not leave known data behind `u8` blobs and raw offset casts. Recover named,
  typed fields as evidence accumulates, then convert all touched callers to
  member access. Keep padding only for ranges whose contents are still unknown.
- Express original-binary ABI checks with `DECOMP_ASSERT`, which is disabled
  for host builds. Do not use an original 32-bit byte size or field offset for
  runtime allocation, iteration, or access: use typed fields and `sizeof` so
  the host ABI remains correct when pointers are wider.
- Prefer a forward declaration over a second empty definition. Run
  `bazel run //scripts/checks:check_duplicate_definitions` when changing shared types.

## 6. Stub and diagnostic conventions

Generated placeholder bodies usually omit parameter names and return a neutral
value. Hand-maintained stubs may name parameters, cast them to `(void)`, and use
`UNIMPLEMENTED("reason")`. Match the surrounding file's style.

`UNIMPLEMENTED` and `LOG_*` produce host diagnostics and compile away in target
builds. `__FILENAME__` is currently a correct repository-relative path such as
`src/legoapi/world/world.cpp`; the former `src/src/` prefix bug is fixed.

Host-only diagnostic commands belong under `src/host/harness/`. Keep asset tools
general: `bazel run --config=native //src:run_native -- load list [filter]`
lists DAT entries and `bazel run --config=native //src:run_native -- load
extract <dat-path> <output>` extracts any entry
(`saga_native.exe` on Windows). Do not add sequence- or level-specific
extraction code to target translation units.

Platform adapters belong under `src/host/platform/`. Prefer implementing an
imported API (`slCreateEngine`, `eglSwapBuffers`, or
`glCompressedTexImage2D`) or selecting a declared platform interface such as
`NuInputDevicePS`. Do not provide a strong host definition of a portable game
or engine symbol; reconstruct that symbol in its original translation unit.

Prefix host-only internal functions and storage with `host` (for example,
`host_read_frame` or `g_host_texture_hashes`). Imported or platform-interface
entry points keep their declared names because the linker-facing ABI requires
them.

## 7. Globals and data placement

Shared globals generally live in `src/globals.cpp` with declarations in
`src/globals.h`; subsystem-local globals stay with their subsystem. Preserve
the original section:

- zero/uninitialized objects normally land in `.bss`
- nonzero constant initialization normally lands in `.data`
- vtables and const pointer tables commonly land in `.data.rel.ro`

Writing `= 0` does not create dynamic initialization or a guard. Confirm the
target symbol class with `nm` before changing initialization.

For initialized registries and pointer tables, preserve the complete original order,
terminator, per-entry flags, and section placement. The target is unstripped, so recover the
table bytes and pointed-to symbols directly as described in `06-target-binary.md` instead of
constructing a minimal table for whichever script or host path happens to run today.

## 8. Authoring checklist

1. Find the exact target symbol and demangle it if needed.
2. Resolve its current source owner with `rg`.
3. Check the effective target command with `bazel aquery`.
4. Preserve linkage and mangling; use ABI types deliberately.
5. Reconstruct the body using the codegen patterns in `02-codegen.md` and the
   mismatch catalog in `07-diagnostics.md`.
6. For a provisional body, use the local stub convention and correct return type.
7. Use `bazel run //scripts:objdiff_cli -- SYMBOL` for the focused diff.
8. Run `bazel test //scripts/checks:checks` before commit.
