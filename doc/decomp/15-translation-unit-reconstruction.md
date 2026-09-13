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
- Original `gizobstacle.cpp` local symbols join the registration callbacks,
  eight-entry update table's static callbacks, and the 64-byte trigger array
  with its count. The latter two were incorrectly exported from a separate
  current file. Merging the `gizmo/gizmos/gizmos_gizobstacles.cpp` functions
  into the owner under `gizmos/object/gizobstacle.cpp`, retaining `-O3`,
  makes those trigger variables file-local and reconstructs the 93-byte
  `_GLOBAL__sub_I_gizobstacle.cpp` initializer. The similarly named
  `gizmo/object/gizobstacle.cpp` containing `InitPaintPuzzle` remains separate;
  the initializer name alone is not ownership evidence. Across merge plus
  rename, all 18 previously exact obstacle functions stay exact, the
  initializer rises 29.41% to 99.35%, and `GizObstacles_AddTrigger` rises
  40.40% to 71.30%. Whole-binary fuzzy matching moves 44.5710% to 44.5703%
  because the large, already low-scoring proximity/reset/update bodies
  currently lose some fuzzy alignment. Their remaining codegen differences
  require body-level diagnosis, not a reversal of the evidenced static-data
  ownership. Target build, symbol coverage, checks, and 120-frame Map smoke
  pass.
- Original `gizturret.cpp` local symbols contain the turret callbacks, output
  name buffer, registration static, and six function-local vectors. Its three
  adjacent `.data` words are `gizturret_rapid_fire_rate`,
  `gizturret_test_ang`, and `turret_gizmotype_id`, formerly split across
  `gizmos/traps/gizturrets.cpp` and
  `gizmo/gizmos/gizmos_gizturrets.cpp`. Merging those sources under the singular
  `gizmos/traps/gizturret.cpp` owner and retaining `-O3` restores that data
  order and brings `_GLOBAL__sub_I_gizturret.cpp` from 0% to 99.35%.
  The secondary file's duplicate empty `GizTurret_ReadAnimSetData` definition,
  marked `__used__`, was removed; the real reader remains in the owner and
  improves 16.80% to 23.96%. `GizmoTurret_GetOutputName` becomes exact, no
  exact match is lost, and whole-binary fuzzy matching rises 44.5703% to
  44.5722%. Target and WASM builds, symbol coverage, checks, and 120-frame Map
  smoke pass. The remaining small Load/Reset body-score declines need ordinary
  source-level investigation.
- Original `gizrandom.cpp` has one short text run containing the callback set,
  `createGizRandom`, and `GizRandom_RegisterGizmo`, plus the registration-local
  static and one initializer. `createGizRandom` was stranded in a default
  `-O0` file with unrelated random functions, while the callback owner is
  `-O3`. Moving only that function into the callback owner and dropping unused
  heavy includes from the residual file leaves one initializer instead of
  two. `createGizRandom` improves from 13.96% to 99.98%; whole-binary fuzzy
  matching rises 44.5722% to 44.5783%, with no regressed or lost exact
  functions. The sole remaining initializer scores 99.35%.
- Original `gizbuildit.cpp` has one long text run across three current `-O3`
  files, with `CalcAveragePosAndRad` between BuildIt functions in that run.
  That function was in `-O2` `misc/utilities.cpp`; moving it to the
  BuildIt owner and declaring it in the BuildIt header changed no function
  scores. The three source bodies were then consolidated in the singular
  `gizmo/object/gizbuildit.cpp` owner at `-O3`, leaving one 281-byte
  `_GLOBAL__sub_I_gizbuildit.cpp` at 99.40% rather than three separate
  initializers. `LEGOCONTEXT_BUILDIT` remains in a minimal separate file:
  its original word is in the `LEGOCONTEXT_*` data table, not BuildIt's own
  adjacent gizmo-ID/debounce/wobble-height data. Across the merge, fuzzy
  matching moves 44.5784% to 44.5785%, six functions improve and two decline
  slightly (`ReleaseBuildIt` 99.66% to 99.19%, `SetToStart` 59.63% to
  59.54%); no exact function is lost. Target build, four checks, and 120-frame
  Map smoke pass. The context-table owner and body-codegen differences remain
  to be reconstructed.
  A subsequent API sweep moves the remaining BuildIt call-site declarations
  into `gizbuildits.h`; it changes no function scores and passes the target
  build, checks, and 120-frame Map smoke.
- Original `gizspecial.cpp` combines the callback run, `createGizSpecial`,
  `GizSpecial_GetName`, and `GizSpecial_FindByName`. Moving those three
  functions from the unrelated-helper file into the `-O3` owner and replacing
  its guessed prefix pointer with the original five-byte `qaz_` array makes
  `createGizSpecial` exact (99.80% to 100%). The first measured move raises
  whole-binary fuzzy matching 44.5783% to 44.5784%, improves three functions,
  and loses no exacts. Narrowing the residual file's includes removes its
  duplicate initializer; the original has one. The empty `FindByName` body is
  still unresolved and requires real implementation, not a TU-layout trick.
- Original `giztimers.cpp` has one contiguous callback-to-registration run,
  including `createGizTimer` immediately after Load. Moving that already-exact
  creator from the separate `gizmo/object/giztimers.cpp` file into the `-O3`
  trigger owner, then renaming that owner to the original plural basename,
  preserves its exact match and restores the sole 93-byte initializer to
  99.35%. Whole-binary fuzzy matching is neutral across the complete move and
  rename; the temporary 0% initializer after the move confirms why the
  basename correction must be measured as part of the same unit.
- Original `nuvertexformat_android.cpp` has one large
  `NuGetVertexDeclaration` body and a file-local vertex-format pool/count.
  Its current default `-O0` body has a frame/local layout unlike the original
  optimized, stack-aligned descriptor loop. A controlled per-TU comparison
  raises this function from 9.62% at `-O0` to 23.00% at `-O2` and 36.79% at
  `-O3`; no other function score changes. The `-O3` setting is retained,
  raising whole-binary fuzzy matching from 44.5785% to 44.5985%. The
  remaining body mismatch must be addressed through real source/layout work.
- Original `gizmoblowups.cpp` has one callback-to-blowup text run beginning at
  `0x004b7540` and ending before the pickup TU at `0x004bedb0`. Its local
  block owns the callback functions, `NewBlowup_RegisterGizmo::addtype`,
  `Blowup_OutputName`, the blowup name table/count, and one 93-byte dynamic
  initializer. The current `gizmos/object/newblowup.cpp` and
  `gizmo/object/gizmoblowups.cpp` were both `-O3` fragments of that run;
  merging them under the original `gizmoblowups.cpp` basename removes a
  redundant current initializer while retaining the original-named one at
  99.35%. Fuzzy matching rises 44.598870% to 44.599873%: four functions
  improve, including `GizBlowup_InitSingleTerrain` (67.33% to 77.15%),
  none regress, and no exact match is lost. A measured follow-up replaces
  supported local cross-TU declarations with owner headers and leaves every
  score unchanged. Target build, four checks, symbol coverage, and 120-frame
  Map smoke pass. This is not yet the complete original TU: original-run
  functions remain in the `-O2` gizmo wrapper and several other files, while
  the merged current source also contains out-of-run helpers. Those require
  separate ownership and optimization tests.
  A controlled optimization trial for the still-separate
  `gizmo/gizmos/gizmos_newblowup.cpp` wrapper changes its setting from `-O2`
  to `-O3`, matching the optimized shape of the original run. Fuzzy matching
  rises 44.599873% to 44.620518%: `GizmoBlowup_Opponent` improves 11.72% to
  55.18%, `GizmoBlowup_Hit` 11.50% to 38.59%, and `GizmoBlowup_Target` 79.83%
  to 88.99%; nothing regresses and all exact bodies remain exact. Target
  build, four checks, symbol coverage, and 120-frame Map smoke pass.
  A subsequent measured merge moves all seven wrapper functions into the
  original-named `gizmoblowups.cpp` owner at `-O3` and removes the redundant
  source. All three exact wrapper bodies and the 93-byte initializer score
  are preserved. Only `GizmoBlowUp_Hit` declines slightly (38.59% to 38.55%),
  leaving aggregate fuzzy matching effectively unchanged at 44.6247%.
  Target build, four checks, symbol coverage, and 120-frame Map smoke pass.
  Out-of-run functions and data in other owners remain separate work.
  `SetLevelExBlowupFunc` was an empty stub in `episode.cpp`, whereas the
  original stores a non-null callback. Restoring that behavior makes it exact
  (35% to 100%); moving it into the BlowUp owner then preserves the exact
  match. `SetLevelExBlowupFlags`, `GetLevelExBlowupFlags`, and their
  `EXBLOWUPFLAGS` word also move from `level.cpp`/`globals.cpp` into the owner
  without changing either previously exact body. The owner header now exports
  these APIs and callback pointers to call sites. Original `.bss` places
  `GizmoBlowUp_NoTargetFn`, `GizmoBlowUpOpponent_Behind/Range2`,
  `GizmoBlowUp_SfxFn`, `GizmoBlowup_TransformDrawFn`, `BlowupExFunc`,
  `EXBLOWUPFLAGS`, and `GameBlowUpBlownUpFn` in that order, 16 bytes apart.
  Moving the last two misplaced callbacks into the owner and arranging the
  eight source definitions in the reverse declaration order emitted by GCC
  4.7 restores that object-local sequence without affecting function scores.
  `CheckLostDataFn` belongs to a separate original data block next to
  `Game_CompletionSave`; moving it to the current globals owner is score
  neutral. At this stage the registration-local `addtype` still followed the
  name table in the current object's `.bss`, unlike the original.
  Placing `NewBlowup_RegisterGizmo` directly after `GizmoBlowups_TotalScore`,
  their original text order, puts the registration-local `addtype` before the
  name table and count in the current `.bss`, as in the original. This gains
  one exact body (`GizmoBlowupLateUpdate`, 99.67% to 100%) but slightly lowers
  two other bodies and aggregate fuzzy matching (44.625084% to 44.625072%);
  the initializer remains 99.35%. This is a measured ownership/data-order
  step, not evidence that the BlowUp TU is fully reconstructed. The
  out-of-run `GizmoBlowup_TransformDraw_Game` at `0x001deb30` follows
  `Transform_TargettedByObj` at `0x001dea90` in the original, before the
  next `Ledges_*` run. It now lives beside that function in the `-O3`
  `transform.cpp` owner, with a shared API header instead of ad-hoc external
  declarations. Its 78.423% body and whole-binary matching are unchanged;
  target build, four checks, zero missing symbols, and 120-frame Map smoke
  pass. This measured series leaves fuzzy matching at 44.625072% and 4,632
  exact functions.
  `GizmoBlowupResetNameTable` at `0x004beb70` is immediately before the
  name-table lookup in the original BlowUp run. It was in `gizmo_sys.cpp`
  with a second, unused-by-lookup 1024-byte table and count, so level loading
  reset different storage from the lookup. Moving Reset into the BlowUp owner
  and removing the duplicate gives the reset and lookup one file-local table.
  Resetting the table before its count naturally retains the original code
  sequence while placing the table before the count in the owner's `.bss`,
  following the registration-local `addtype`. No function score changes; target
  and WASM builds, four checks, zero missing symbols, and 120-frame Map smoke
  pass. `GizmoBlowupCreateStuff` and `SetGizmoBlowUpTarget` remain out-of-run;
  their neighbors require a broader TU investigation before moving them.
  Two more ordinary functions in the original BlowUp text interval were
  stranded outside its owner. `UpdateMidPos` at `0x004b9ff0` was in default
  `-O0` `move.cpp`, whereas its original neighbors and the BlowUp owner are
  `-O3`. Moving its unchanged body and putting the exported declaration in
  the BlowUp header raises that function from 56.69% to 87.75% and the whole
  binary from 44.6251% to 44.6312%, with no exact loss. Its antinode call
  now uses a header from the antinode owner instead of a linker-only local
  prototype. `PlayAnim` at `0x004ba9e0` also lies between `UpdateMidPos` and
  the BlowUp early-update run; moving its unchanged body from `animation.cpp`
  to the same owner leaves its 98.16% score and all other scores unchanged.
  These are ownership corrections, not proof that the BlowUp TU is complete.
  Target and WASM builds, four checks, zero missing symbols, and 120-frame
  Map smoke pass after both moves.
- Original `gizmopickups.cpp` has a 41-function text run from `0x004bedb0`
  to the next turret owner at `0x004c28a0`. Its local block includes
  `GizmoPickups_CollideList`, registration-local `addtype`, and file-local
  `GizmoPickupSys`; adjacent data places `COINMAGNETSCALE`, `COINMSGTIME`,
  and `GizmoPickups_Collide2DFn` in the same owner. The pickup-specific
  functions and data formerly in `gizmo/gizmos/gizmos_gizmopickups.cpp` were
  moved into the existing `-O3` `gizmos/fx/gizmopickups.cpp` owner, while
  singular-`gizmopickup.cpp` `Pup_*` callbacks, `CollectCoin`, and
  `SetOnOff` remain separate. `GizmoPickups_CollideList` was placed before
  `GizmoPickups_AllocateProgressData`, following their original text order.
  Whole-binary fuzzy matching moves from 44.600426% to 44.598564%: no exact
  function is lost, `GizmoPickups_Collide` improves 45.747% to 50.374%, but
  `GizmoPickups_CollideList` declines 67.204% to 56.413%. Moving its
  definition to the original-relative position did not alter those scores.
  Objdiff shows changed branch/register allocation and a reordered collision
  tail, so the cause remains unresolved; TU-level optimization context is a
  hypothesis, not a proven explanation. The target build, four checks, and
  120-frame Map smoke pass. Do not count this partial merge as a reconstructed
  complete TU; other functions and data in the original address interval
  remain in separate current owners.
  A second measured stage moves `GizmoPickups_InitSys` and
  `SpecialMiniKits_Reset` out of `items/collect/minikits.cpp` into the same
  pickup owner. Both now use one file-static `_ZL14GizmoPickupSys`, matching
  the original local symbol at `0x00668660`; the redundant minikits-file
  pointer is removed. `InitSys` precedes registration and
  `SpecialMiniKits_Reset` follows it, as in the original text run. Fuzzy
  matching rises from 44.598564% to 44.598870%, with no lost exacts:
  `GizmoPickups_InitSys` stays 99.857%, `GizmoPickups_Reset` improves
  47.075% to 49.094%, and `SpecialMiniKits_Reset` declines 98.519% to
  96.019%. The latter objdiff has six differing instructions, including the
  moved pointer's GOT displacement and register/compare variants; it shows
  no changed calls or source-level behavior. Target build, four checks,
  symbol coverage (zero missing), and 120-frame Map smoke pass. The
  `SpecialMiniKits_Reset` register differences and other pickup-run functions
  still need ordinary source/TU reconstruction.
  A later body-level comparison found that the original collision filter
  tests pickup-type bit `0x10`, whereas the reconstructed collision code used
  challenge-mode bit `0x20`. The original Charkit type-table byte is `0x21`,
  confirming `0x20` remains the challenge-mode bit; a distinct collision bit
  was added without changing that table. The original collision helper also
  stores its two condition flags as 32-bit integers and places the manual
  distance test on the fall-through path, with the sphere helper on the other
  branch. Restoring those ordinary source types and equivalent branch form
  raises `GizmoPickups_CollideList` from 56.41% to 78.49%. Its accesses to
  pickup types now use the original file-static `GizmoPickupSys` pointer,
  shared with `GizmoPickups_InitSys`, instead of a fixed game-global system;
  that structural/behavioral correction leaves the helper at 77.01% because
  register/layout differences remain. `GizmoPickups_Collide` and
  `GizmoPickups_TotalScore` also use the pointer as in the original, with no
  further function-score change. The full set of these body/data corrections
  raises fuzzy matching 44.620518% to 44.624714%, keeps all exact matches,
  and passes target build, four checks, symbol coverage, and 120-frame Map
  smoke.
- Original `gizspinner.cpp` has a single text run of spinner callbacks and
  implementation functions, with one initializer and adjacent spinner data.
  The two current `-O3` sources were consolidated under the original basename
  in `gizmos/door/gizspinner.cpp`. The spinner ID, failure-state array, and
  output-name array now use the original values and storage types: `-1`,
  `{-1, -1, 0}`, and `"100% Complete"`, respectively. The merged initializer
  rises from 0% to 99.35%, `GizSpinner_GetTargetPoints` and
  `GizSpinner_GetOutputName` improve slightly, and all 11 prior exact bodies
  remain exact. Whole-binary fuzzy matching rises approximately 44.5985% to
  44.6004%; no function regresses. The compiler still emits these data
  definitions in a different order from the original, so their remaining
  address/layout difference is not considered solved. Target build and
  120-frame Map smoke pass.
- Original `jumping.cpp` places file-local `BigJump_JumpAction_Default` at
  `0x004ed380` immediately before `BigJump_LandAction_Default` at
  `0x004ed410`, then continues with the ordinary jumping functions. Both
  defaults are in its `_GLOBAL__sub_I_jumping.cpp` local-symbol block. The
  initialized `BigJump_LandActionFn` and `BigJump_JumpActionFn` pointers are
  adjacent in original `.data` at `0x00668c9c` and `0x00668ca0`. Those four
  definitions were stranded in `characters/motion/move.cpp`, despite the
  consumers and the original-named owner already being in
  `actions/movement/jumping.cpp`. Moving them together preserves local
  linkage, the original relative function and pointer order, and the
  existing public pointer declarations in `characters/motion.h`. Both files
  use the same default optimization. The exact jump callback remains exact;
  the land callback remains 99.86%, and no exact function is lost. Four
  neighboring functions shift slightly in fuzzy score, with virtually no
  whole-binary change. Target and WASM builds, four checks, zero missing
  symbols, and 120-frame Map smoke pass.
- Original `zipup.cpp` has a contiguous text run from `ZipUp_ActivateRev`
  at `0x001d1440` through `ZipUps_DrawLines` at `0x001d4910`, before the
  detonator run starts at `0x001d4b40`. The file-static
  `ZipUp_GetStartPoint` at `0x001d1500` sits among the zip-up callbacks in
  that run, while its callers were stranded in a separate movement source.
  Stage 1 combines the `-O3` callback source
  `gizmos/door/zipups.cpp` with the `-O3` movement source
  `props/objects/zipup.cpp`, retaining the original `zipup.cpp` initializer
  basename and the helper's local linkage. No function body is changed; the
  unnecessary `__used__` marker on the now-referenced helper is removed.
  Whole-binary fuzzy matching rises from 44.6312% to 44.6320%, with
  `ZipUp_GetStartPoint` improving 87.16% to exact and no exact match lost.
  `ZipUps_Load` declines 0.10 percentage points; `ZipUps_Reset` and
  `ZipUps_DrawLines` improve slightly. The obsolete callback-file `-O3`
  override was removed; target build, four checks, zero-missing symbol
  coverage, and 120-frame Map smoke pass. This stage is partial because
  `InitRopeMtl`, `DrawRopeSingle`, and adjacent rope data are still in
  separate sources.
  Stage 2 moves `InitRopeMtl` at `0x001d3d30` and `DrawRopeSingle` at
  `0x001d3e40` into the same zip-up owner, between registration and
  `ZipUps_DrawLines` in the original text run. Their `ropemtl` global and
  `DrawRopeSingle` function-local `ROPELEN` and `ropedif` follow them; the
  latter two now use natural function-local static declarations, matching
  the original local-symbol names. The resulting object has the original
  relative `.data` order `zipup_gizmotype_id`, `ZipUpHookOffset`,
  `zipup_outputName`, `ropedif`, and `.bss` order `ropemtl`, registration
  `addtype`, `ROPELEN`. The separate original `rope.cpp` initializer remains
  with `DrawRopeCurved` at `0x00502e50`, moved unchanged from the render
  catch-all to a real rope owner with a public header; its body is still an
  unfinished stub, not evidence of a matched implementation. Original
  `LEGOACT_WHIP_SWING_*` data is far from the zip-up data block and remains
  a separate ownership question. Stage 2 preserves all exact matches and
  changes whole-binary fuzzy matching only from 44.6320% to 44.6319%.
  `ZipUp_FindNearest` improves 0.10 percentage points;
  `DrawRopeSingle` falls 0.03 and unrelated `DrawStillScreen` falls 0.24,
  both still body-level differences. Target build, four checks, zero-missing
  symbol coverage, and 120-frame Map smoke pass.

### Animation action-ID data

The original `.data` span `0x00667198..0x0066723b` contains 82 consecutive
two-byte `LEGOACT_*` globals, all initially `-1` except `LEGOACT_LUNGE = 1`.
Their global `.symtab` entries are consecutive and reverse-address ordered.
This data follows the game-object data and precedes the 40-word context-ID
table; the nearby initializer and local `.bss` sequence is
`gameobjects.cpp`, `animation.cpp`, `contexts.cpp`. Together these are strong
evidence for an original animation-owned action-ID table, although global
data order alone cannot prove source ownership.

The 63 previously defined action IDs were moved, unchanged, to
`characters/motion/animation.cpp` and declared in its `animation_ids.h` owner
header. The 19 absent IDs were then reconstructed with their original `-1`
defaults. Each corresponding assignment in `InitGameAfterConfig` was checked
against a store through the original GOT before restoring it; these are real
runtime assignments, not data-layout filler. The current GCC object now
contains exactly the 82 original action symbols, with every relative `.data`
offset matching `original address - 0x00667198`. `LEGOACT_LUNGE = 1` remains
the one non-`-1` default. Both stages preserve whole-binary function matching
at 44.631924%, with 4,633 exact functions. This is an ownership hypothesis
supported by layout and startup order, not a claim that all bodies currently
in `animation.cpp` belong to the original TU.

The adjacent context-ID table starts at `0x0066723c` and consists of 40
four-byte `LEGOCONTEXT_*` words, all originally `-1`. Their contiguous global
symbol order, adjacent initializer/local-vector block, and separation from
the following level data support a distinct `contexts.cpp` owner, still an
inference rather than definitive TU provenance. The 34 existing definitions
were moved there; six absent definitions were restored. Every original
`InitGameAfterConfig` store was checked before restoring the six corresponding
commented assignments. The original `LEGOCONTEXT_BUILDIT` word also starts at
`-1` and receives `0x2d` at runtime; the previous source had these phases
reversed. All 40 current context words now have exact original-relative
offsets in their object.

A separate GOT-store audit of `InitGameAfterConfig` confirmed 13 more
previously commented assignments for already-defined action/context IDs;
those stores and their original values are restored. The only remaining
commented ID assignment is a duplicate `LEGOCONTEXT_JUMP = 0` (the active
store already exists), so it remains inactive. No ID assignment was enabled
on the basis of a comment alone.

`contexts.cpp` has only its necessary owner header. Including broader headers
produced a 99.35% initializer score in a trial, but no current context code
needed them, so that score-shaping include was removed. The original TU's
`VuVec_*` locals and initializer remain unreconstructed. `-O3` is a provisional
per-file candidate supported by data order and the original initializer
shape; substantial original-owned functions are still needed to verify it.
The principled table move preserves whole-binary function matching at
44.631924% and 4,633 exact functions.

### Scattered animation-source bodies

The former animation catch-all also contained functions at widely separated
original text addresses. `NeedsPretendAnim` at `0x00150af0` is directly before
`MovePlayer_VEHICLEDIRECTIONAL` in the original vehicle-movement run. Moving
its unchanged body to `move.cpp` (both current units `-O2`) keeps its 100%
match; its three character-ID dependencies are declared in the character
owner header instead of in the source body.

`ReadInstAnimBlockDlist` at `0x002d9780` and `ReadInstAnimBlock` at
`0x002d9940` sit between `StateAnim*`, `ReadInstanceIDs`, and `NuGScn*`
functions in the original scene-processing run. Their unchanged bodies and
private layout helper now live in `nugscn.cpp`, in original address order,
with the scene API declarations in `nugscn.h`. The allocator declaration is
also in that owner header. The recipient's `-O3` changes
`ReadInstAnimBlock` from 97.34% to 99.79%; ordering the pair raises
`ReadInstAnimBlockDlist` from 70.96% to 71.11%. No exact match is lost and
whole-binary fuzzy matching moves to approximately 44.6322%.

The original `gcutscn.cpp` local-symbol block contains named file-local
cutscene functions on both sides of `EvaluateJointOrientationMtx` at
`0x00437000`. Its unchanged body now lives in `gcutscn.cpp`, using its real
cutscene header and scene/joint dependencies. Both current owners use `-O2`;
the moved body and all existing gcutscn scores remain unchanged. This
local-block evidence is stronger than address adjacency alone.

`RedirectAnim` at `0x0045fe80` sits between the character-name lookup helpers
and `CharScenes_Init` in the original text run. The original `characters.cpp`
local block contains the corresponding `CharScene_Area`, icon-scene, and
variant data, while the following `charconfig.cpp` block begins later. Its
unchanged body has moved into `characters.cpp` immediately before
`CharScenes_Init`, retaining its previous score. With no unrelated bodies
left in `animation.cpp`, its unused catch-all includes were removed. The
original animation initializer is not reconstructed by fake includes; it
remains an explicit gap while the 82-word data table stays intact.

An unchanged `NuGScnUpdate` move to `nugscn.cpp` was also measured and
reverted. Its own 42.33% score did not change, two unrelated functions lost
tiny fractions of a point, and the mixed-family address run did not supply
independent TU evidence. The render owner stays in place pending stronger
evidence or body reconstruction.

### Android graphics-scene platform unit

The original `nugscn_android.c` has a contiguous ordinary-text run from
`NuIOSBindVAO` at `0x002fd760` through `NuGSceneProcessCrossFade` at
`0x002fefbd`. Its local-symbol block contains the file-static
`NuIOSBindVAO`, `UploadDataToGLBuffer`, and `PreWarmGeomsAndBakeVAOs`; the
upload and fixup bodies also embed the full original Android source path in
their critical-section calls. These independent clues make the platform TU
owner substantially more certain than name affinity alone. The `.c` suffix
does not imply that the reconstructed file should be compiled as C: several
symbols in the run have C++ linkage.

The unchanged bind/upload/prewarm/fixup bodies were moved out of the render
catch-all into `android/nugscn_android.cpp`. `NuGScnReadTexturesPS`,
`NuGScnCreatePS`, and `NuGScnDestroyPS` moved from the generic `nugscn.cpp`
owner; `NuGScnRndr3` came from render, and `NuGScnFixupTIDsPS` and
`NuGScnRestoreTIDsPS` from `nurndr_plain.cpp`. The file's definition order
now follows their original address order, including the two existing small
platform stubs. Declarations needed by other units are in the Android scene,
scene, and iOS display-list owner headers, rather than new source-local
link-time `extern` declarations. `NuReadGraphicsData` remains in render: its
original local block belongs to the preceding `nu3d_includes.cpp` unit.

The measured whole-binary fuzzy match rises from 44.6322% to 44.6873%
(about +0.0551 percentage points), with three additional exact functions and none
lost. `NuIOSBindVAO` and `NuGScnRndr3` become exact; upload, texture-ID,
prewarm, and fixup routines make large non-exact gains. Five unrelated render
or hub functions move down by at most 0.20 percentage points; these small
layout-sensitive changes are recorded, not treated as evidence that the
platform ownership is wrong. Reordering the two existing platform stubs to
their original positions changes no function score.

This is still an incomplete reconstruction. `NuGSceneSetCrossFade` was moved
unchanged from `nucore_plain.cpp` into the evidenced Android owner, immediately
before its two crossfade neighbors. It has no call sites in the original
executable or current tree. Its original 24-byte body accesses the second and
third stack arguments, but the current stub has an incorrect no-argument
signature. The exact source types and semantics cannot be established from
those instructions alone; no dummy argument code is added for score. The
owner move lowers this unfinished stub from 46.67% to 31.11%, reducing the
whole-binary fuzzy score by about 0.0001 percentage points without losing an
exact match. `NuGScnDestroyPS` is likewise an empty body versus a
substantial original function. Ownership gains for these symbols should not
be mistaken for body matching.

### Character-name lookup pair

The original `characters.cpp` run contains `CharIDFromName` at `0x0045fd80`,
`CDataFromName` at `0x0045fe00`, then `RedirectAnim` and `CharScenes_Init`.
Its initializer/local-symbol block owns the character-scene static data, so
the run and block together support the plural `characters.cpp` owner.
Moving unchanged `CharIDFromName` from the singular `character.cpp` into
that owner leaves its 99.67% score and all other scores unchanged.

The neighboring `CDataFromName` was an empty, incorrectly `void` stub in
`legoapi_misc.cpp`. The original body iterates `CHARCOUNT`, compares the
name to `CDataList[i].file`, and returns the matching record or null. Its
observed 0x4c stride and `file` offset 0x0c agree with the declared
`CHARACTERDATA` layout. The real lookup now resides between `CharIDFromName`
and `RedirectAnim`, with its exported return type in the character owner
header. Its measured match rises from 11.74% to 99.67%; whole-binary fuzzy
matching rises from 44.6873% to 44.6894%, with no other function score
changes and no exact-match transitions. The remaining fractional difference
is not papered over with an instruction-shaping change.

### AI-message translation unit

The original `gizmessage.cpp` text run at `0x004b68e0..0x004b6ef0`
interleaves five message-gizmo callbacks, the generic message-system APIs,
and the gizmo registration. Its local-symbol block ends at
`_GLOBAL__sub_I_gizmessage.cpp` and contains those callbacks, the
registration-local `addtype`, `GetOutputName`'s local return buffer, and a
five-byte `gizaimessage_prefix` object. The current callbacks were split into
`gizaimessage.cpp`, while the generic APIs were in `gizmessage.cpp`; both had
independent pointer-valued prefix statics. Both sources used `-O3`.

The unchanged bodies now live in the original-named owner in original text
order. One `static char gizaimessage_prefix[] = "msg_"` serves both sides,
restoring its original five-byte object type and its `.data` position directly
after `gizaimessage_gizmotype_id`. The emptied trigger source/header and its
obsolete per-file option were removed. Cross-TU declarations now use the
base message header and the actual global/AI allocator owner headers; the
allocator header is also included at its definition.

This move raises `CreateGizAIMessageSys` from 97.65% to exact and preserves
all previous exact functions. `ResetGizAIMessageSys` and registration improve
slightly; `CheckGizAIMessage`, whose body is already far from matching,
declines from 12.68% to 8.32%. Whole-binary fuzzy matching therefore changes
from 44.6894% to 44.6892%. The ownership and data evidence are retained;
the Check body needs ordinary source-level reconstruction, not a TU or
attribute workaround.

### Grapple helper run

The original `grapples.cpp` initializer/local block and ordinary-text run
place `Grapple_SetPlayerTargetPoint` at `0x004d6eb0`,
`Grapple_SetTargetMom` at `0x004d6f90`, and `Grapple_SetRotOrder` at
`0x004d7090`, after the grapple registration/dynamic-movement functions and
before `Grapple_LookAtPos`. These three real bodies had remained in a
separate `-O3` gizmo wrapper while the original-named `grapples.cpp` owner
was also `-O3`. Their unchanged definitions now sit in that owner in address
order, using its existing public header. The move changes no function score:
`SetRotOrder` stays exact, `SetTargetMom` stays 99.96%, and
`SetPlayerTargetPoint` stays 84.13%. The residual wrapper still contains
unfinished stubs, including an empty `__used__` local helper; those are not
moved or used as a matching shortcut.

### Mini-cutscene owner and original basename

The original `gizminicut.cpp` run puts `GizMiniCut_GetGuid` at `0x004d95b0`
directly between `GizMiniCut_Load` and `MiniCut_RegisterGizmo`, with a
`_GLOBAL__sub_I_gizminicut.cpp` local-symbol block. The already-exact GetGuid
body was isolated in a separate `-O2` source; the callback/registration owner
was `-O3` under the generic `minicut.cpp` name. Moving the unchanged body
into its original position at `-O3` preserves its exact match. The private
offset-only data shim was replaced with a real `MINICUT::guid` field at the
verified 0x1a offset, retaining the 0x30-byte ABI. The empty extra source and
its obsolete option entry are removed.

Renaming the owner source to the original `gizminicut.cpp` basename, with its
existing `-O3` option moved to the new path, makes the genuine initializer
score 99.35% rather than 0%. Whole-binary fuzzy matching rises from
44.6892% to 44.6912%, with no body regression or exact-match loss. The
public `minicut.h` stays under its semantic API name; filename spelling
changes no function body.

### Menu versus model customiser code

The current `menus/screens/customise.cpp` mixed the original menu-oriented
run at `0x001b8f20..0x001bbbb0` with a separate customiser/model run at
`0x0049fb20..0x004a29c0`. The latter has an original
`_GLOBAL__sub_I_customiser.cpp` block with file-local
`Customiser_PieceAvailable_Default` and `CustomSetData` symbols. This proves
the basename and boundary, but not the exact historical directory; the new
`characters/core/customiser.cpp` location is a semantic choice.

Four implemented functions in the second run—`Customiser_NextPieceLeft`,
`Customiser_NextPieceRight`, `Customiser_ResetModelTextureIDs`, and
`Customiser_CopyDefaultPiecesToSave`—now live in that owner in original
relative order at the same effective `-O2`. Their bodies are unchanged. A
real `customiser.h` supplies the call sites and definition, replacing the
touched source-local declarations. Matching is unchanged: the two near-exact
piece selectors retain their scores, both texture/save helpers remain exact,
and no other function changes. The remainder of the original run includes
unfinished stubs and absent local data, so this is a partial TU extraction;
no fake initializer or forced-emission marker is added to imply completion.

### Render-device and GLES2-extension boundaries

The original local-symbol sequence has three distinct initializers:
`_GLOBAL__sub_I_NuRenderDevice.cpp` (271 bytes),
`_GLOBAL__sub_I_NuRenderDevice_gles2.cpp` (93 bytes), and
`_GLOBAL__sub_I_NuGLES2Extensions.cpp` (93 bytes). Each has its own nearby
file-local `VuVec_*` block. This rules out merging the separately implemented
extension routines into the current base device TU merely because their text
addresses are adjacent. The unchanged three-function/six-pointer extension
unit was instead renamed to the original `NuGLES2Extensions.cpp` basename at
the same `-O2` setting, with its used initialization API in an owner header.
The rename changes no function score. Its original initializer is still
absent; no unrelated header is included just to manufacture one.

The original ordinary-text run at `0x002a74d0..0x002a7d9d` consists of the
GL error hook, render-thread selection, critical-section methods and
wrappers, buffer swap, resize, and `NuRenderDevice::Initialize`. The latter
three contain literal full paths ending in `NuRenderDevice_gles2.cpp`, and the
critical-section family uses the same thread-context state. That path plus
the separate original initializer/data block supports an Android GLES2
source owner, although the exact boundary with the preceding base methods is
not proved by address alone. These unchanged suffix bodies now live in
`android/NuRenderDevice_gles2.cpp` in original text order at `-O2`, alongside
`gt_glContextIndex` and `g_nextGLContextIndex`. `g_renderDevice`, its
constructor, and preceding lifecycle methods remain in the base source.
Their cross-TU declarations are in `NuRenderDevice.h`, including host users
of the thread-context state; no new weak/used attributes were introduced.

All previously exact device and extension functions remain exact. Only
`NuRenderDevice::Initialize` moves from 70.28% to 70.17%, a roughly
0.000026-point whole-binary decline (44.691166% to 44.691140%). The base
initializer remains 61.14% and the new GLES2 initializer has no natural
current counterpart yet. The split is retained for its path/data evidence;
missing genuine initialization and remaining body differences are explicit
follow-up work, not reasons to shape a score with unused includes.

### NuScreen source owner

The original `_GLOBAL__sub_I_NuScreen.cpp` has a distinct file-local
`VuVec_*`/rodata block. The complete original `NuScreen` text run
(`0x000eed80..0x000eee60`) and `NuScreen::ms_instance` already belong to the
single current `nuscreen.cpp` source at `-O2`; all eight reported function
entries are exact. Renaming that owner to `NuScreen.cpp` preserves every
function score and the optimization setting. The original initializer is
not recreated by the basename alone, and no dummy local state was added.

### Platform and utility source owners

The original `_GLOBAL__sub_I_NuPlatform.cpp` names the owner of
`NuPlatform::ms_instance` and the implemented platform functions. Renaming
the existing lowercase source to `NuPlatform.cpp` preserves its `-O3`
setting and every function score. Its original 93-byte initializer is still
missing because the current source does not naturally emit the associated
file-local `VuVec_*` block.

The original text places `UtilGetTime`, `UtilGetFrameStartTime`, and
`UtilFrameStart` consecutively between the `Transporter.cpp` and `Ftp.cpp`
runs. The intervening `_GLOBAL__sub_I_Utilities.cpp` local block also owns
`frameStartTimeMS` and `frameStartTime`. These three unchanged functions and
the two statics now live in `gamelib/util/Utilities.cpp` at their former
effective `-O3`, while the GroupBuffer family remains in its existing
source pending stronger ownership evidence. `Network.cpp` uses the new
owner header instead of a source-local link-time declaration. The split
preserves all function scores, but does not yet emit the original `VuVec_*`
initializer. No unused dependency was added to force one.

The preceding original `Message.cpp` run contains
`NetMessage::RaiseError` at `0x0052b6b0` and `NetMessage::DebugPrint` at
`0x0052b6d0`, followed immediately by the Network/stream run. These two
already exact bodies and `NetMessage::sm_poolMessageData` now share
`gamelib/util/Message.cpp` at `-O3`. `theSession` stays in `Network.cpp`:
the member methods' text adjacency does not establish ownership of that
separate global. Its declaration is in the shared type header, replacing a
function-local link-time `extern` in `RaiseError`. The original
`_GLOBAL__sub_I_Message.cpp` remains absent and was not synthesized.

The original `Stats.cpp` text run follows the network-object run: seven
implemented `NetSmallStats`, `NetSample`, and `NetStats` methods occupy
`0x00535790..0x00536619`, followed by an as-yet-unimplemented weak
`NetSmallStats::Update`. Their separate 93-byte initializer and local
`VuVec_*`/stats-string block confirm a distinct owner. The seven existing
bodies now live in `gamelib/util/Stats.cpp` at `-O3` in original order,
without score changes. The missing initializer belongs to substantive
stats rendering code, not a file-rename trick; this extraction does not
claim those low-scoring bodies are matched.

The separate owner also made the small `NetSample` methods practical to
verify. The original arithmetic is four 32-bit lanes, with an unsigned
per-lane maximum. Ordinary explicit field operations at `-O3` match both
arithmetic operators exactly and bring `Max` to 99.95%, raising overall
matching from 44.691140% to 44.693275% without regressions. No vector
intrinsics, assembly, attributes, or forced initializer were needed.

### Application-state and platform lifecycle boundaries

The original local-symbol sequence separates `NuApplicationState.cpp`,
`NuPlatform.cpp`, and `nudevicespecs.cpp` into adjacent initializer blocks.
The four exact application-state methods form the complete text run
`0x000f0f80..0x000f0fbf`; they now live in `NuApplicationState.cpp` at
their previous effective `-O2`, with no score change. No unused include was
added to manufacture its absent initializer.

`NuPlatform::{Exists,Destroy,NuPlatform,~NuPlatform}` were stranded in
`nucore.cpp` between `Create` and `SetCurrentPlatform` in the original
address sequence. They now join the real `NuPlatform.cpp` owner at `-O3`.
The old `nucore/NuPlatform.h` declared an incompatible second class; it now
forwards to the canonical platform header, which declares the moved
methods. The original `Destroy` frees `ms_instance` and clears it; that
ordinary lifetime body raises the function from 30% to exact.

`NuDeviceSpecs::Exists` and its destructor similarly rejoin the existing
`nudevicespecs.cpp` owner. A per-file optimization trial found `-O3`
strictly better than `-O2`: both give the same five non-initializer scores,
but the genuine initializer reaches 99.35% only at `-O3`. Against the
previous default setting, `Create`, `Destroy`, and the constructor become
exact, while `DetermineDeviceSpecs` rises from 0% to 86.62%. Together with
`NuPlatform::Destroy`, this batch raises overall matching from 44.693275%
to 44.7138% with four newly exact functions and no regressions.

### Thread and memory-pool bodies in their existing owners

The original `NuThreadManager::CreateThreadSuspended` has its own local
priority map and returns the newly created `NuThread`, with the requested
stack size unchanged and `is_suspended` set. Reconstructing that ordinary
body in the existing `NuThreadManager.cpp` owner raises it from 12.35% to
99.68%. The real header and the legacy stand-in declaration now agree on
the pointer return type; this does not unify the stand-in's other types.

The original `NuMemoryPool::AddPage` allocates a 20-byte `Page`, initializes
its data pointer and counters, links it under the pool mutex, then adds its
size to free bytes. Its field offsets agree with the original `PageAlloc`
and `ReleaseUnreferencedPages` methods. Reconstructing that layout/body in
the existing pool owner raises `AddPage` from 8.94% to 99.98%. The
`page_list_stable` field must be observable across the lock interval; making
it `volatile` preserves the original false/true stores and also brings
`GetPagedBytes` from 93.79% to exact. These are source-level data and
concurrency semantics, not forced-emission markers or instruction hacks.
Overall matching rises from 44.713820% to 44.720432% without regressions.

The neighboring original `NuMemoryPool.cpp` run also contains
`InterlockedPush` and `InterlockedPop`, previously empty stubs in
`numemory.cpp`. Their ordinary compare-and-swap loops now live beside
`InterlockedAdd`/`InterlockedSub` in the pool owner. `Pop` returns the
removed free block, as the original return register shows; both canonical
and legacy declarations now use that pointer return type. The reconstructed
pair reaches 99.83%/99.93% respectively, without assembly, intrinsic
vector types, or forced-emission attributes. The complete thread/pool batch
reaches 44.721450% overall with no regressions.

The remaining `NuMemoryPool` methods had still been collected in
`numemory.cpp`. They now share `NuMemoryPool.cpp` with the implemented
atomic/page methods at the same `-O2` setting. Their existing bodies are
unchanged, so this owner correction leaves every function score unchanged;
large unimplemented release, merge, and allocation routines remain
explicitly unfinished.

The 37 `NuMemoryManager` definitions formerly collected in `numemory.cpp`
likewise now live in `NuMemoryManager.cpp`, preserving their bodies and `-O2`
setting. The move is matching-neutral. The original manager-table and page
visitor bodies reveal that each visitor's first vtable slot is its visit
method, not a virtual destructor. Correcting the canonical interfaces and
implementing the two locked traversals raises `VisitManagers` from 11.35%
to 99.97% and `VisitPages` from 12.31% to exact. The thread, pool, and
manager batch reaches 44.725765% overall, with seven improved functions
and no regressions. The four checks, zero-missing-symbol check, target and
WASM builds, and 120-frame Cantina smoke test pass.

### Memory-manager debug, context, and large-bin methods

The original `NuMemoryManager.cpp` text run provides a coherent set of
low-scoring debug methods. Reconstructing the flag-gated backtrace copy,
packed context field, name/context setters, and block validator makes seven
of these methods exact. The original extended header has 32 backtrace slots
followed by a count at offset `0x8c`; both debug accessors return a count or
context ID, correcting their former `void` declarations. `UnTouchAllBlocks`
now walks each page's block headers and clears the touch flag on allocated
blocks, reaching 91.82%.

`PushContext` allocates a 16-byte context followed by its name, records the
used-block count, and links it above the current context. Its original
allocation label contains the embedded `nu2api.2013/numemory/NuMemoryManager.cpp`
path and source line 1627. That recovered source-location metadata is kept
as program data, not a forced link or compiler option; the body reaches
92.87%.

The three linked-list large-bin sorting methods were empty stubs. Their
reconstructed merge sort uses ascending block size, takes the right list on
ties, and repairs backward links after sorting. The merge is exact; the
recursive splitter reaches 93.15% and the outer sort 66.68%. These remaining
instruction differences are not claimed to be solved. The whole batch raises
matching from 44.725765% to 44.7572%, with 11 improved functions, seven
newly exact, and no regressions.

### Pool construction and animation-data owner

The original `NuMemoryPool` constructor initializes a recursive mutex,
stores its handler, block size, and debug name, clears its 0x400-byte bin
region, and links the pool into a global list. Those fields now have names
in the canonical header; the trailing page-statistics fields are identified
in the next checkpoint. The 0x440-byte target allocation size is preserved.
The constructor is exact. `NuMemory`'s three pool factories, two destroy paths, and MEM2-to-MEM1
page transfer now use the original returns, argument order, and source-location
allocation labels: four are exact, two exceed 99.8%. The pool destructor
unlinks itself and destroys its mutex, reaching 75.23%; its called page-release
routine is addressed in the next checkpoint. Both global visitor walks are
static as their one-argument ABI shows, and both now match exactly. The pool's two linked-list
merges are exact; the recursive sorts reach 90.98% each.

Six animation-data definitions previously collected in
`nu2api_nucore_misc.cpp` now live in the existing `nuanim.cpp` owner. The
public and internal cross-TU declarations are in headers, not local linker-only
`extern`s. Two animation bodies improve from this move, while an unchanged
neighboring `NuHGobjEvalAnimBlend2Root_3` slips 89.10% to 88.92% from the
new code layout; no behavioral change or safe source correction was found.
The ANI3 size walk and pointer relocation are now genuine bodies in that
owner, reaching exact and 99.69% respectively. Overall matching rises from
44.7572% to 44.8024%: 31 functions improve, 17 become exact, and the one
minor neighboring regression is recorded rather than concealed. Target and
WASM builds, four checks, zero-missing-symbol coverage, and a 120-frame
Cantina smoke test pass.

### Pool page lifecycle and Android online owner

The original pool's trailing words at offsets `0x424`–`0x42c` record visited,
released, and recycled page counts; the page-release body also clears the
words at `0x434`–`0x43c`. The constructor's 0x440-byte object size is
unchanged. `ReleaseAllPages` now follows the original force-release and
free-byte accounting path exactly. `ReleaseUnreferencedPages` locks the pool,
sorts pages and all 256 free lists by address, counts free blocks per page,
then releases or recycles fully free pages through the handler. It reaches
58.72%, with its remaining code-generation differences still open.

`PageAlloc` now returns the pointer indicated by the original ABI and
updates the page offset and allocation count. Its page-selection loop is
counterintuitive: when the head lacks space, the original skips later pages
that *do* fit and rotates the first later page that also lacks space before
asking the handler to allocate another page. The reconstruction preserves
that observed control flow rather than substituting a conventional first-fit
allocator. The body reaches 35.38%; this low score is explicitly unfinished.

The original `_GLOBAL__sub_I_nuonline_android.cpp` and a contiguous
`0x274620`–`0x2749e0` text run identify the Android NuOnline unit. Its
profile functions and PS wrappers now share `nuonline_android.cpp` at `-O3`,
while the distinct generic `nuonline.cpp` owner remains separate. Four PS
wrappers become exact. Two presence-mode wrappers slip from exact to 97.11%
after the owner/optimization change. A separate BSS/data audit found six
local `VuVec_*` objects immediately after the online globals. Including the
real `nuvuvec.hpp` header in this owner emits those six objects and recovers
the original static initializer to 99.35% without fabricating an initializer
or adding a per-function compiler override. Immediately before the vectors,
the original BSS also has `g_signinUIFinishedDisplaying` (4 bytes) and
`g_changedSettings`, `g_changedProfiles`, `g_signedinProfiles` (16 bytes
each). Their exact element types and use sites still need mapping before
adding source definitions.

This combined checkpoint rises from 44.7258% to 44.8228%: 39 functions
improve, 22 become exact, and three small regressions remain visible (the two
online wrappers and the unchanged animation neighbor). The target and WASM
builds, four checks, zero-missing-symbol audit, and 120-frame Cantina smoke
test pass.

### Cutscene locator-animation owner

Four locator-animation bodies now share `nugcutscene_anim.cpp` in their
original address order: `NuGCutLocatorCalcMtx_3`, `NuGCutLocatorCalcMtx`,
`NuGCutLocatorIsVisble_3`, and `NuGCutLocatorIsVisble`. The three
function-local filter arrays in the `_3` visibility routine move with their
owner, and the cross-TU APIs are declared in `nugcutscene.h`. The recovered
unit retains `-O2`. Target matching is exactly neutral at 44.8209% before
the independent online initializer recovery, with
no function-score changes; this is a structural correction, not a claimed
body improvement.

The adjacent `NuATanf`/`NuATan2f` pair is another possible Android float
unit, but both bodies are already exact in their current owner. Their
original initializer bears a `.c` filename while the callable symbols are
C++-mangled, so the compilation-language boundary needs stronger evidence
before relocating them.

### NuQT helper unit

The original ELF has eleven contiguous quadtree routines at
`0x261783`–`0x2626f4`. The first helper phase creates a dedicated
`nuqt.cpp`/`nuqt.h` owner with the evidenced 0x38-byte header and 12-byte
entry layout. `ElOverlaps`, `RemoveData`, and `AddNode` now have real bodies
there instead of attribute-retained placeholders in unrelated RTL/Ogg files.
The two address-fix helpers also move out of `nuquat.cpp` and remain exact.
The Y-axis comparisons in `ElOverlaps` use the original field order; neutral
dimension names avoid asserting a conventional min/max order not supported by
the observed code.

This phase raises matching from 44.8228% to 44.8301% with no regressions:
`AddNode` is exact, `RemoveData` is 99.76%, and `ElOverlaps` is 96.13%.
The public NuQT bodies and the larger insertion helpers remain unfinished;
in particular, an independent disassembly audit confirmed a latent original
`InsertData` null-destination copy at `0x261883`–`0x261889` when a leaf has a
null data pointer. Its normal-game reachability is not established, so the
insertion cluster needs a coherent behavior audit before implementation. The
target and WASM builds, four checks, zero-missing-symbol audit, and 120-frame
Cantina smoke test pass.

The original `NuQTRead` and `NuQTWrite` wrappers now use the real NuFile API,
align or unfix/fix the stored pointers, and update the caller's buffer cursor.
Their source-level success/failure control flow reaches 77.44% and 85.29%,
respectively. `NuQTCreate` now aligns and reserves the caller's arena,
initializes the 0x38-byte header and entry/data regions, and reaches 93.86%.
The original returns zero on both the successful and insufficient-space paths;
the reconstructed body preserves that observed behavior. The three bodies
replace zero-argument export stubs. Insertion remains pending. The combined
target and WASM builds, four checks, zero-missing-symbol audit, and 120-frame
Cantina smoke test pass.

### DDS-functions owner

The original `NuDDSFunctions.cpp` initializer and six local vector constants
accompany the contiguous `NuDDSGetTextureDescription`,
`NuDDSSetTextureDescription`, `NuDDSGetMipLevel`, and `NuDDSGetSize` text run
at `0x52a260`–`0x52ac90`. The four definitions now live together in
`nu3d/NuDDSFunctions.cpp` at `-O3`, with API declarations in `nutex.h`.
Including the real vector header recovers the static initializer to 99.35%.
The existing texture-description body remains at 82.13%; the other three
bodies initially remained low-scoring stubs. `NuDDSGetSize` now follows the
original description/mip query, palette adjustment, and header-size addition,
raising that wrapper from 6.46% to 83.94%. Its return type is `i32`, as the
original return register shows. The mip routine now has a real source-level
body and four DDS-specific format tables verified byte-for-byte against the
original ELF. An independent disassembly audit caught and corrected its
compressed block-size expression and the minimum width/height orientation.
Separating the original compressed and uncompressed loops raises it to 8.53%
without other function regressions. It remains a low match despite these
behavior checks, so further reconstruction
and focused runtime validation are required before treating it as a faithful
match or relying on it for all texture formats. `NuDDSSetTextureDescription`
now writes the 128-byte DDS header, including the original 26-entry format
dispatch, dimension/mipmap flags, and cubemap caps. Its `nutexturetype_e`
parameter is a fixed-underlying integer enum rather than the former empty
struct placeholder, restoring the original value-passing ABI. An independent
disassembly audit found no semantic mismatch; the body reaches 85.98%.

The unit move and size wrapper raise overall matching from 44.8301% to
44.8362% with no
function regressions. The target and WASM builds, four checks, zero-missing-
symbol audit, and 120-frame Cantina smoke test pass.

The subsequent NuQT wrappers and DDS bodies raise the combined overall
score to 44.8580% with six improved functions and no regressions. The same
target/WASM/check/symbol/Cantina gates pass on the combined tree.

### Android rain owner

The original `nurain_android.c` initializer follows the five contiguous rain
entry points at `0x52b134`–`0x52b347`. Their code now lives together in
`nu3d/android/nurain_android.c`, compiled as C++ to match the original
C++-constructed vectors despite the `.c` suffix. The original
`NuRainSetFall(float)` clamps the requested value to [0, 1]; it replaces an
incorrectly typed empty
zero-argument stub and rises from 15.56% to 99.85%. The other four entry
points remain exact. The original `.data` bytes for `NuRainKey`, `testrain`,
and `NuRainOldY` match the compiled object, while four rain globals and the
first six local 16-byte `VuVec` objects recover their adjacent `.bss`
ownership. The second nearby six-vector group follows FMV globals and is not
part of this TU. A real rain header replaces the terrain caller's local
linker-only declaration. The combined score reaches 44.8598%, with seven
improvements and no regressions.

Using the original filename recovers the genuine
`_GLOBAL__sub_I_nurain_android.c` at an exact 100% match, with no rain-body
regression. This is a source-language configuration, not a fabricated symbol.

### Android FMV owner

The three contiguous `NuFmvInit`, `NuFmvPlayV`, and `NuFmvPlay` bodies at
`0x52b370`–`0x52b42a` move unchanged from the miscellaneous core file to
`nu3d/android/nufmv_android.cpp`; all three remain exact. Five FMV globals
occupy the original `.bss` region immediately after rain, followed by the
second six local `VuVec` objects. The separate
`_GLOBAL__sub_I_nufmv_android.cpp` at `0x0e2740` directly initializes those
six objects and now matches at 99.35% under the evidenced `-O3` setting.
This takes the combined score to 44.8617% with eight improvements and no
regressions.

### Screen-dump wrapper

The standalone `NuPs2VideoScreenDump` body at `0x52ac90`–`0x52ad54` now
lives in `nucore/nuvideo_dump.cpp` with a real seven-argument declaration in
`nuvideo.h`. The original formats `<base><face>.bmp` for a nonnegative face;
otherwise it probes `<base>.bmp` and then numbered `%03d` suffixes until a
filename is unused. It calls the front-buffer getter but does not write an
image. The implementation preserves that observed behavior rather than
inventing a capture path. An `-O3` trial reaches 95.21% (from 7.50%) and the
public header replaces the editor caller's local declaration. The combined
score reaches 44.8654% with ten improvements and no regressions.

The post-screen-dump target/WASM builds, four checks, and zero-missing-symbol
audit pass. The ordinary native smoke invocation currently cannot open the
new 3.09 GB `res/main.1060.com.wb.lego.tcs.obb`: its 32-bit file open returns
`EOVERFLOW` before engine startup. Without changing either asset, the same
current native binary passes 120 healthy Cantina frames from a temporary
working directory using the repository's pre-existing 1.31 GB `.obb.bak`
and copied save fixture. This is a fixture-size limitation, not a successful
test against the newly replaced OBB.

### NuFileDevice path and handle storage

The original `NuFileDevice` method run at `0x318970`–`0x3190ef` is already
co-located in `nufiledevice.cpp` at `-O3`, but five methods were empty or
incorrectly typed. `AllocDirectoryHandle`, `FreeDirectoryHandle`,
`GetDeviceByType`, `GetDeviceFromPath`, and `AddPathRule` now follow the
observed locking, copied-path allocation/freeing, device selection, and
ordered path-rule behavior. The real internal type declares the original
0x180-byte rule table, 0x80-byte directory-handle table, and four-byte Bionic
mutex static. Their original target storage sizes are verified. The five
bodies rise to 88.08%, 100%, 93.50%, 44.14%, and 62.59%, respectively;
26 formerly exact neighboring bodies remain exact. Overall matching reaches
44.8894%, with no regressions. The target/WASM builds, four checks, and
zero-missing-symbol audit pass; the current native binary also passes 120
Cantina frames using the saved compatible OBB fixture described above.

### Texture-animation program persistence

The existing `nutexanim.cpp` `-O3` unit contains a contiguous persistence
cluster around `0x2cb270`–`0x2cb696`. Four zero-argument placeholders now
have their original signatures and real bodies, declared in the public
`nutexanm.h`: `NuTexAnimProgCreate` reserves a program and instruction span,
`Destroy` unlinks/frees owned programs, `Write` serializes the fixed header
plus used instructions, and `Read` loads and links a program with its ownership
flag. The create buffer path does not align its cursor, matching the original;
the read path likewise retains the original's unchecked allocation failure.
Scores rise to 87.60%, 99.97%, 100%, and 99.96% for Create, Destroy, Write,
and Read. Overall matching reaches 44.9024%, with four additional
improvements and no regressions.

The combined post-persistence target/WASM builds, four checks, and
zero-missing-symbol audit pass. The current native binary again advances 120
healthy Cantina frames with the saved compatible OBB fixture.

### CRC16 table owner and shader caller

The original `CRC16.cpp` has one global instance, a 256-entry table, three
contiguous methods at `0x30e1e0`–`0x30e3b6`, and a separate static
initializer. The constructor now builds the original CCITT polynomial table;
`hash` and `hashInverse` are static methods taking only data and length, as
confirmed by their original call sites and stack arguments. Both use the
table with `0xffff` initial state, forward and reverse byte order respectively.
An independent `123456789` check gives `0x29b1` forward and `0x84df` reverse.
The measured `-O3` unit produces 91.11%, 61.22%, and 44.85% for the three
bodies, with the original table and instance storage sizes. The shader-key
generator now calls these real methods rather than maintaining a second CRC
table in `nushadermanager_plain.cpp`; its score rises from 33.54% to 41.47%.
The combined score is 44.9170%, with 25 improvements, three newly exact
functions, and no regressions against the preceding commit.
