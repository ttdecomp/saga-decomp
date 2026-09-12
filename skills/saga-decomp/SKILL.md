---
name: saga-decomp
description: Work safely on saga's Android x86 matching decompilation, including ABI signatures, GCC 4.7 codegen, objdiff diagnosis, source placement, and documentation maintenance. Use for reconstructed game code, matching analysis, or decompilation docs; do not use for unrelated generic C++ work.
---

# Saga matching decompilation

Preserve the repository's matching contract and make the smallest useful
change. Inspect current state before editing and do not overwrite unrelated
work.

## Start from live state

1. Run `git status --short` and note existing changes.
2. Read `doc/decomp/00-index.md`, then only the chapters relevant to the task.
3. Read `doc/source-structure.md` before moving a function or file.
4. Use live Bazel authorities instead of copied generated data:
   - source membership and outputs: `src/BUILD.bazel`
   - target configuration: `.bazelrc`
   - per-source optimization: `bazel/android_per_file_copts.bazelrc`
   - effective commands: `bazel aquery --config=target`

## Build and compare

```bash
bazel build --config=target //src:saga_target
bazel run //scripts:objdiff_cli -- MANGLED_SYMBOL
bazel test //scripts/checks:checks
```

The objdiff helper reports the original on the left and current build on the
right. Read `-` as original behavior missing from the current code, `+` as
extra current code, and `~`/`>` as original/current operand variants.

Locate source ownership with `rg`. If membership or options are ambiguous, use:

```bash
bazel aquery --config=target \
  'mnemonic("CppCompile", deps(//src:saga_target))' --output=commands
```

## Non-negotiable rules

- Preserve exact symbol name, linkage, namespace/class, signature, and ABI
  types. `int` and `long` are both 32-bit here but mangle differently.
- Preserve the source file's optimization mode. No override means `-O0`.
- Plain names normally require `extern "C"`; `_Z...` names use C++ linkage.
- The target uses GCC 4.7 from NDK r8e, i386 PIC, SSE arithmetic, and disabled
  exceptions/RTTI.
- Definition order, data section, literal first-use order, signedness, field
  offsets, and static initialization can all affect emitted bytes.
- Native/WASM behavior is diagnostic and must not leak into the target.
- Use `__attribute__` only for a verified, documented linkage, layout,
  platform, or runtime requirement; never merely to raise a matching score or
  force an instruction pattern. Explain the necessity near its use.
- Do not add `regparm`, `fastcall`, `thiscall`, or similar calling-convention
  attributes as matching shortcuts. Record unresolved ABI discrepancies.

Read `doc/decomp/02-codegen.md`, `04-types-abi.md`, and
`07-diagnostics.md` before changing source to chase an assembly shape.

## Working boundaries

- Follow the user's requested scope. A review does not authorize a source or
  build-system rewrite.
- Use `/tmp` for isolated compiler experiments.
- Use the NDK r8e driver and documented target options for codegen experiments.
- Use `apply_patch` for edits and preserve concurrent modifications.
- The optional repository hook runs the checks and, when the reference binary
  is present, regenerates the committed Pages report. Do not run unrelated
  formatters or generators automatically.

## Verification

- documentation: scan for stale paths and test documented commands;
- one symbol: `scripts/objdiff-cli.py`, `nm`, and `objdump -dr`;
- symbol surface: `bazel run //scripts/checks:check_symbols -- --list`;
- source/build changes: rebuild `//src:saga_target` and run
  `bazel test //scripts/checks:checks`;
- native-specific changes: also build `//src:saga_native` on the relevant host;
- browser changes: also build `//src:saga_wasm`.

Report exactly what was run and what could not be verified on the current host.
