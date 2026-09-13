# Reconstructing the original translation units

> Investigation and execution plan, checked on 2026-09-13. This is not a
> proposed mass rename or a claim that the original TU map is known.

The objective is to put each reconstructed definition in approximately its
original source file and compile that file with its original optimization
level. This is a matching task: source ownership, local linkage, definition
order, static initialization, and per-file options can change the emitted
binary even when a function body is unchanged. The original ELF and the live
Bazel compile actions are the authorities; filenames and domain prefixes are
clues, not substitutes for them.

## Baseline and available evidence

- `bazel aquery --config=target` currently finds **523 target TUs**: 510 C++
  and 13 C. Effective optimization is 173 implicit `-O0`, 2 `-O1`, 112 `-O2`,
  and 236 `-O3`. One `-O3` TU also uses `-fPIE`. The declared overrides live in
  `bazel/android_per_file_copts.bazelrc`; `src/BUILD.bazel` uses a source glob
  and creates one archive per top-level engine component.
- The committed report has a 44.5299% whole-binary fuzzy match. It assigns
  12,314 of 13,454 reported functions to one current Bazel unit, marks 235
  ambiguous, and leaves 905 unassigned. These are *current-source* ownership
  figures, not evidence of original TU boundaries.
- The original `res/libTTapp.so` retains `.symtab` and `.strtab` but has no
  DWARF or `STT_FILE` entries. Its `.init_array` has 327 entries. There are
  325 `_GLOBAL__sub_I_*` symbols with 320 distinct basenames. Of these known
  original basenames, 94 have no same-named target source file today. This is
  a lower bound on original TUs, because files without dynamic initialization
  need not have a `_GLOBAL__sub_I_*` symbol. The suffixes include 37 `.c` and
  283 `.cpp` basenames; a filename suffix alone does not prove which compiler
  language mode was used.
- The binary embeds 21 distinct original absolute `.c`/`.cpp` paths, including
  `saga/androidbatman.cpp`, `legoapi.saga/screen.cpp`,
  `nu2api.saga/nu3d/android/NuRenderDevice_gles2.cpp`,
  `nu2api.saga/nu3d/android/nugscn_android.c`, and
  `nu2api.2013/nusound/nusound_buffer.cpp`. These are unusually strong
  directory/name clues, but appear only where a path was emitted into data.
- In the original local-symbol portion of `.symtab`, repeated `.LC0` names and
  ordered `_GLOBAL__sub_I_*` entries expose apparent input-object blocks.
  For example, local entries 1–40 end at `_GLOBAL__sub_I_Controllers.cpp`,
  and entries 41–52 end at `_GLOBAL__sub_I_squish_pch.cpp`, with literal
  numbering restarting. File-local `_ZL*` symbols in those blocks identify
  static data/functions, while `_ZZ*` names can identify the enclosing
  function directly; references from ordinary code to those addresses can
  connect a normal `.text` run to its original TU. For instance, the first
  block contains `_ZZ17NuIOS_YieldThreadE5count` and the wake-render statics.
  This is an observed property of this link, not an
  ELF guarantee. The [ELF symbol-table specification](https://gabi.xinuos.com/elf/05-symtab.html)
  guarantees local symbols precede global/weak symbols, but does not promise
  recoverable object boundaries among those locals.
- Normal function addresses are a strong sequencing clue. As a calibration,
  11,826 uniquely mapped function addresses in the *current* linked target
  were sorted by address and labeled with their known Bazel source owner.
  Excluding initializer/thunk names, 375 of 382 units with at least five
  mapped functions occupied one consecutive run; 99.92% of their functions
  fell within each unit's largest run. This measures our link, not the
  original's unknown build order, but supports testing contiguous address
  intervals as primary candidate TU boundaries.
- A check against the current linked ELF, where `STT_FILE` remains available,
  found 555 file blocks including dependencies. Of those, 63 have no useful
  non-section local symbol, and another 130 have locals but no
  `_GLOBAL__sub_I_*`. Removing the `STT_FILE` labels therefore makes many
  boundaries intrinsically ambiguous. Do not claim a complete original TU
  manifest from constructor names or local-symbol runs.

The original source roots visible in embedded paths (`saga`, `legoapi.saga`,
`nu2api.saga`, and `nu2api.2013`) differ from the current domain-oriented
tree. Reconstruct meaningful file ownership first; matching every historical
directory spelling is secondary unless it changes emitted strings or build
behavior.

## Evidence ranking and uncertainty

For each proposed original TU, record the candidate filename, symbols, source
language, optimization level, and evidence separately. Rank evidence as:

1. An embedded original full path, if it can be tied to the function or data
   being moved.
2. A basename from `_GLOBAL__sub_I_*` supported by an ordered local-symbol
   block, literal-name reset, file-local static symbol, and references to
   that static from ordinary code. A function-local `_ZZ*` name may make the
   association especially direct. Initializer addresses are not ordinary
   `.text` endpoints: startup sections are collected apart from normal bodies.
3. A consecutive run of ordinary `.text` functions, checked for references
   to the same local static/data block and for coherent definition order.
   Treat weak, COMDAT, linker-generated, and separately collected sections as
   exceptions rather than forcing them into the interval.
4. `.init_array` order as a link-order clue, not a complete address map.
5. Name/domain affinity and current source placement. Useful for finding
   candidates, weak as proof of original ownership.
6. Code-generation evidence for optimization level, calibrated with the exact
   NDK r8e GCC 4.7 command and *multiple substantial functions* from the
   candidate TU. Tiny stubs, thunks, and shared helpers are poor classifiers.

The [GNU linker documentation](https://sourceware.org/binutils/docs/ld/Input-Section-Wildcards.html)
explains how input-section order can influence output order, but a function's
address alone does not identify its source file. The
[GCC 4.7 optimization manual](https://gcc.gnu.org/onlinedocs/gcc-4.7.2/gcc/Optimize-Options.html)
also makes clear that `-O` levels enable different passes; matching a prologue
is a useful filter, not a complete optimization-level proof. Record high,
medium, or unresolved confidence rather than forcing every symbol into a TU.

## Reconstruction workflow

The symbol ledger for step 1 is reproducible with:

```sh
PYTHONPATH=. python3 scripts/restructure/generate_original_tu_map.py
PYTHONPATH=. python3 -m unittest scripts.restructure.test_original_tu_map scripts.restructure.test_calibrate_tu_map
```

The first command needs only `res/libTTapp.so`. To compare with a current
build, pass explicit `--current <ELF> --units <JSON>` to either script. The
JSON is a list of `{ "source": "...", "object": "..." }` records or an object
with a `units` list (for example, `matching.json`); object paths may be
relative to the repository root. The scripts neither invoke Bazel nor define
Bazel targets. `calibrate_tu_map.py` requires both inputs.

The first script writes `.work/original-tu-map.json` (ignored by Git). It
records each original symbol, and optionally each current object symbol, with
its symbol-table index, section, address, size, type, binding, and visibility;
aliases and zero-sized symbols remain separate. The ledger uses `.symtab`; a
defined-name comparison
with `.dynsym` found no dynamic-only defined names in this reference ELF.
Optional current source/object pairs and optimization metadata come only from
the supplied JSON; `null` means the manifest did not provide an optimization
value. Original-to-current candidate IDs are exact-name, same-type joins
within the same local/nonlocal binding class; they identify a
possible **current owner**, not a proven original TU. The generated artifact
is deliberately not a hand-maintained build authority.

On the `fabus1184/restructure` baseline, the ledger contains 32,596 named,
defined allocated original symbols: 14,541 in `.text`, 9,384 in `.rodata`,
6,625 in `.bss`, 1,796 in `.data`, and 250 in other allocated sections. The
type split is 13,459 `FUNC`, 8,916 `OBJECT`, and 10,221 `NOTYPE`. In the
optional baseline comparison, the supplied `matching.json` represents 523
current source/object pairs. There are 325 initializer-delimited
local-symbol blocks and an undelimited tail, 327 `.init_array` entries,
21 distinct embedded source paths, 247 name-derived function-local-static
anchors, and 1,105 same-location/size/type alias groups. Of original symbols,
17,742 have one same-name/type current object candidate, 10,771 have multiple,
and 4,083 have none. These figures are **candidate counts**, not recovered-TU
coverage. For example, the block ending in
`_GLOBAL__sub_I_NuInputDevice_android.cpp` contains a local squish function
and squish lookup tables before that initializer. This proves that assigning
the entire local block to its ending basename would be wrong. Static symbols
remain valuable evidence only when corroborated by text address and usage.
Candidate quality differs sharply by section: `.text` has 12,590 unique
same-name/type current-object candidates, while `.rodata` has only 367 unique
and 8,480 ambiguous, mostly because compiler literal labels repeat. For
writable data, `.data` has 1,232 unique candidates and `.bss` has 3,417.
The ledger exposes these counts per section so text-only progress cannot hide
unresolved data placement.

The separate `.work/tu-map-calibration.json` hides 555 actual `STT_FILE`
records in the current ELF, then checks inferred initializer-delimited blocks
against those records. Among 13,767 local symbols with a preceding file
marker, only 77.12% lie in their inferred block's majority true file; 293 of
363 inferred blocks mix multiple true files. This sharply limits using a
constructor basename as ownership for every preceding local. Among 12,628
uniquely object-attributable current text functions, 98.04% fall in their
owner's largest contiguous address run (for owners with at least five such
functions). Address order is useful evidence, but the remaining splits and
unowned symbols require independent checks. These calibration figures are
not original-TU assignment accuracy; the original has no `STT_FILE` ground
truth.

1. **Capture a reproducible baseline.** Derive original symbol index, address,
   size, binding, type, section, constructor basename, initializer order, and
   embedded paths directly from `res/libTTapp.so`. When comparing a current
   build, supply an explicit source/object manifest and check its provenance
   separately. Save the matching report and exact-match counts for
   comparison. Do not turn a guessed original map into a build authority.
2. **Validate the inference method before using it.** On our built ELF, hide
   `STT_FILE` labels and attempt to reconstruct the known object blocks from
   local-symbol order, `.LC` resets, static references, normal `.text`
   adjacency, and `.init_array`. Measure which blocks and global-address
   ranges are recovered correctly and report ambiguous or
   invisible TUs. This bounds the method's false confidence.
3. **Build an evidence ledger.** Assign original functions to proposed TUs
   only with traceable symbol/address/path evidence. Keep aliases at one
   address together. Use references to file-local statics to test proposed
   address intervals; check vtables, constructors, and literal order as well
   as exported function names. Mark unresolved cases instead of inventing
   filenames or dummy initializers.
4. **Infer optimization per candidate TU.** Sample several nontrivial bodies
   whose types and control flow are understood. Compare their original
   instruction shape against exact-toolchain builds at `-O0`, `-O1`, `-O2`,
   and `-O3`, then confirm the best level with objdiff. Separate an optimization
   mismatch from an incorrect body, ABI type, or source order. Preserve the
   verified `-fPIE` exception; do not propagate it by filename analogy.
5. **Move one evidenced group at a time.** Create or rename the real source
   owner, keep its language and definition order, update the per-file map only
   when justified, and retain the same target/native/WASM source boundary.
   Put cross-TU declarations in the proper exported or internal headers and
   include them at both definitions and call sites. Do not paper over an
   incorrect split with ad-hoc `extern` declarations that merely resolve at
   link time.
   Bazel's source glob will pick up new files, but inspect the effective action
   and object link order after each change. Do not create empty files solely
   to reproduce `_GLOBAL__sub_I_*` names.
6. **Prove the move.** Compare the moved symbols and neighboring functions
   before/after, not just the overall percentage. Rebuild `//src:saga_target`,
   run `//scripts/checks:checks` and `//scripts/checks:check_symbols`, and run
   the 120-frame Map smoke when behavior or shared types change. Check native
   and WebAssembly builds for moves that affect their linkage. Regenerate the
   matching report and investigate *every* lost exact match or material
   regression. CI should pass before merging a batch.

This workflow does not authorize assembly, calling-convention attributes,
visibility/`used` attributes added for score, fake initializers, or other
instruction-shaping shortcuts. Unexplained differences stay documented.

## Pilot queue

| Area | Current evidence | First investigation, not an assumed move |
|---|---|---|
| Original-path anchors in Android/nu2api | Full paths in ELF, but current files differ in some basenames | Map their local and global symbols and test a small, high-confidence file/optimization correction. |
| `src/nu2api/nucore/nucore_plain.cpp` | 487 text symbols, 5,617 lines, current `-O3`; many original `Nu*` basenames | Partition symbol families against original initializer/local blocks; move only a coherent evidenced subset. |
| `src/gameapi/edtools/edtoolsall.cpp` and `edtoolsall_plain.cpp` | 270 and 253 text symbols, both current `-O2` | Identify original editor filenames and prove distinct unit/optimization clusters. |
| `src/legoapi/render/core/terrain_stubs.cpp` | 160 text symbols despite the provisional filename, current `-O3` | Separate implemented terrain behavior from placeholder exports before splitting. |
| `src/legoapi/items/objects/gameobjects.cpp` | 366 text symbols, 7,659 lines, current `-O3` | Build a per-function owner ledger first; cross-domain references make a blind split risky. |
| `src/gameapi/ai/aisys/aisys.cpp` | 413 text symbols, 10,636 lines, current `-O3` | Defer until smaller pilots establish extraction and verification practice. |

The first implementation batch should be the smallest cluster with both a
credible original owner and a testable optimization hypothesis, not simply
the largest catch-all. A TU move is accepted only when symbol coverage and
host builds remain intact and any matching regression is explained. Revisit
link/archive order after ownership and optimization are reliable; changing
linker layout to conceal incorrect source structure would undermine the goal.

The first pilot screen found a credible `nushaderprogram_android.cpp` cluster:
the original local block includes `GetHLSLRegisterIndex` clones and shader
program state, and its normal code contains adjacent `LinkShaderProgram`,
`ValidateShaderProgram`, and `NuShaderProgramCreateIOS` functions. Today these
are in `src/nu2api/nu3d/nushader.cpp`. But the first two functions already
match exactly, while the original also has a missing
`BuildRegisterIndexToUniformLocationMapping` function with a 256-byte
function-local buffer and a 377-byte initializer. The current file instead
has a TU-global buffer and no corresponding initializer. Splitting just the
present functions would risk exact matches without reconstructing the missing
structure, so no move was made. Revisit this group only as a complete TU with
before/after object and whole-binary comparison.

A second plausible cluster is original `timing.cpp`: its unique `TBGAMECOUNT`,
`TBDRAWCOUNT`, `TBPLAYERCOUNT`, and `TBAICOUNT` statics accompany the adjacent
`TBRESET`, `TBOPENFN`, `TBCLOSEFN`, and `TimingBars` text run. The first three
currently live in `supportall.cpp`; `TimingBars` lives in `timing.cpp`, which
also has two exactly matched frame-counter functions. `TBOPENFN` and
`TBCLOSEFN` still have very low body scores, so merely moving their existing
definitions could regress the exact neighbors without a real matching gain.
Before revisiting, reconstruct those bodies and use a proper timing header for
cross-TU declarations instead of scattered local `extern` declarations.

## Measured reconstruction pilots

- `legoapi/items/objects/cable.cpp` was compiled at the default `-O0`, while
  the original has optimized frameless bodies and `UpdateCables` has the
  stack-realignment shape associated with `-O3` vectorization. Setting only
  this source to `-O3` raised whole-binary fuzzy matching from 44.5299% to
  44.5714%: nine functions improved, none regressed, and `InitCables` plus
  `DestroyCable` became exact.
- The original `gizforce.cpp` local-symbol block contains the SFX data,
  `GizForce_FindBestForceTarget`'s 6144-byte function-local array,
  `GizForce_Throw`'s function-local vector, and the registration static. Its
  text run interleaves functions formerly split between
  `gizmos/traps/gizforce.cpp` and `gizmo/gizmos/gizmos_gizforce.cpp`; the
  separate `gizmo/object/gizforce.cpp` belongs to a different address region.
  The two files were merged without changing their bodies or `-O3` setting.
  Matching then measured 44.5710%: all 15 pre-existing exact force functions
  remained exact, `GizForceSFX_returnsfx` improved 99.43% to 99.93%, while
  `GizmoForce_GetOutput` fell 55.30% to 48.26% and
  `GizForce_FindBestForceTarget` fell 33.04% to 32.99%. The `GetOutput`
  objdiff shows changed branch/boolean-result codegen; resolve that from the
  function's actual control flow, not an optimization or attribute shortcut.
  Target build, symbol coverage, checks, and 120-frame Map smoke passed.
