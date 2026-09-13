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
