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

### Android time platform unit

The original six-function time run at `0x270991`–`0x270ba4` is followed by
`_GLOBAL__sub_I_nutime_android.c` and a local `g_startTime` adjacent to six
`VuVec` objects. The current `.cpp` has been renamed to the original `.c`
filename but compiled as C++, preserving the mangled platform helpers.
`NuTimeGetMicrosecondsPS` moved from a miscellaneous empty stub into this
unit and now uses the same observed `clock_gettime` and microsecond conversion
as the neighboring exact `NuTimeGetTicksPS`. Including the shared vector
definition reconstructs the original six local objects and makes the named
initializer exact. Expressing ticks-per-second as the original 64-bit value
makes that helper exact too. Existing `NuGetCurrentTimeMilisecondsPS`,
`NuTimeGetTicksPS`, and `NuTimeGetTime` remain exact, and `NuTimeInitPS`
remains 99.85%. The caller uses the existing `nutime.h` declaration instead
of a local linker-only declaration.

In the same interval, the shader-manager destroy and save-folder wrappers
have been moved from placeholders to their existing shader-manager unit,
with a real header and the original 256-byte save-folder global. Their scores
reach 93.79% and 100%. The two bounded CRC string loops in their existing
`crc.cpp` unit now test the character before the length bound, as the original
does, raising their matches to 47.93% and 35.15%. The combined measured
score is 44.9237%, eight improvements, four newly exact functions, and no
regressions against the preceding commit.

The original `.c` names require the repository's clang-tidy aspect to parse
those two files as C++, just as the actual compile actions do. The first PR
run exposed this mismatch for `nurain_android.c` in Linux target/native lint;
`linters.bzl` now explicitly follows the real language for both reconstructed
`.c` units. This is a tooling language correction, not a source-level match
override. Target, native, and WASM clang-tidy have passed locally with the
change.

### Android material state and thread unit

The embedded `numtl_android.cpp` source path in `NuIOSMtlInit` anchors the
original material run around `0x29bf60`–`0x29c9b0`. The current mixed
`nuiosdl_gl.cpp` had held that initializer, four file-local shader programs,
its shader-source arrays, cull state, render-state setter, material callback,
and refraction locals alongside unrelated geometry callbacks at `0x293xxx`.
Those material definitions now live together in the existing `-O2`
`numtl_android.cpp` unit; the geometry callbacks stay in their former unit.
The duplicated distortion-texture ID is now one original file-local object
shared by initialization and the callback. Real shader and texture headers
carry the cross-unit declarations, without changing the shader/refraction
locals to external linkage. `NuIOS_SetVertexFormat` becomes exact and
`NuIOS_SetCullMode` rises from 0% to 99.85%; `NuIOSMtlInit` reaches 81.04%,
`NuMtlSetRenderStatesPS` 34.02%, `NuIOSDLMtlCallback` 13.37%, and the named
material initializer 32.04%. The nearby exact geometry callbacks do not
regress. `NuIOSDLReflectionCallback` remains a separate empty stub pending
its own evidence-backed reconstruction.

The original `bgproc_android.cpp` initializer constructs another six local
`VuVec` objects before its thread-state storage. Restoring the shared vector
definitions in that correctly named unit raises its initializer from 40.27%
to 98.27%, without changing its runtime bodies or their scores.

The same original unit owns `bgPostRequestV`, previously implemented in
startup's `main.cpp`. Its direct access to `cur_pi`, `procinfo_pool`,
`g_bgCritSec`, and `events` makes their original file-local linkage possible
again. The symbol table confirms those four objects are local to
`bgproc_android.cpp`, while the public request functions remain declared in
`bgproc.h`. This ownership correction raises `bgThreadMain` from 64.31% to
95.29%, `bgPostRequestV` from 64.91% to 71.73%, `bgPostRequest` from 63.52%
to 72.36%, `bgProcInit` from 93.31% to 99.89%, `bgGetProcActive` from 93.71%
to 99.88%, and the initializer from 98.27% to 99.49%. The semaphore
destructor helper also becomes exact. No function regresses.

The original `nuthread.c` run is explicitly bounded by `VuVecSet` at
`0x271154` and `_GLOBAL__sub_I_nuthread.c` at `0x27167d`. The legacy thread
functions and their critical-section/thread tables move from the mixed
`nuthread.cpp` to that original `.c` name, compiled as C++ as its mangled
helpers and vector constructor require. Its global thread-specific key,
six local vectors, critical-section tables, and legacy-thread tables now
follow the original adjacent `.bss` order; the named initializer is exact.
The thirteen existing exact thread bodies remain exact. Three nearby class
methods at `0x0efc30`, `0x0efdb0`, and `0x0f0d70` move from the remaining
`nuthread.cpp` placeholders to their original `NuThread_android.cpp` class
unit: `SetDebugName` forwards to the base setter, `Resume` clears suspension,
and `GetDebugName` returns the stored name. All three become exact, and the
existing exact class bodies stay exact. The far-away `nu_current_thread_id`
stays in `nuthread.cpp` pending stronger ownership evidence. The lint aspect
also parses this third C++-compiled `.c` unit in its actual language.

Before the varargs move, the combined target measurement was 44.9455%,
eleven improved functions, five newly exact functions, and no regressions
against the preceding commit.
An `-O2` whole-unit trial for `NuThread_android.cpp` improved `NuThreadSleep`
but lowered the near-exact constructor and overall match; its established
`-O3` setting was restored, and the full report returned to the same baseline.

### Original glutils.c texture helpers

The original `glutils.c` initializer follows `CreateSubtractiveTexture` and
`CreateAlphaBlendTexture`, with a local `VuVecSet` and six vector objects in
the same symbol block. Those helpers had been separated into the unrelated
`surfaces.cpp` unit; `CreateSubtractiveTexture` was an empty void stub despite
the original returning a material pointer. They now live in a real
`glutils.c` C++-compiled unit with a shared export header. The bitmap loader
result has its own descriptor type with a pixel allocation pointer at
offset `0x10`; plain `NUTEX` descriptors remain 12 bytes. That distinction
preserves the existing debris-glass initializer while permitting the
original bitmap cleanup in `CreateSubtractiveTexture`.

The immediately preceding `NuPs2VideoScreenDump` is *not* assigned to this
unit merely by address adjacency. It retains its 95.21% match under `-O3`
but drops to 0% under a whole-unit `-O0` trial, whereas the two texture
helpers rise to 99.95% and 93.35%, and the named initializer becomes exact.
There is no evidence of an original per-function optimization pragma, so
the video dump stays in its existing `-O3` source pending stronger ownership
evidence. The `glutils.c` texture unit uses `-O0` and has no matching
attributes or pragmas.

Across the material, thread, background-process, and texture-helper units,
the target report rises from the preceding commit's 44.9237% to 44.9592%:
20 functions improve, seven become exact, and none regress.

### TimeBar shared-state run

The original `0x2d7410`–`0x2d7a28` run contains the TimeBar creation,
destruction, slot, and enable routines together. The low-scoring stubs in
`nucore_plain.cpp` were disconnected from the file-local set list and state
already reconstructed in `nutimebar_plain.cpp`. Moving only that contiguous
run into the existing `-O2` TimeBar owner restores direct access to the
original local state and avoids a linker-only bridge. The earlier
`NuTimeBarInitEx` and adjacent near-exact slot functions stay in place.

Disassembly review also corrected several runtime semantics, not just
matching: `NuTimeBarCreateSet` forwards a colour-array pointer and returns an
integer set ID; full capacity overwrites set slot 15 instead of indexing
past the 16-entry list; heap-owned sets use the engine's allocation/free
manager with the original `"Main"` name; and `NuTimeBarSlotSetEx` applies a
special conversion for slot 6 of set -1. The original `DestroySet` has no
null guard and always clears its list entry, while the raw-microsecond query
reads the selected accumulator without an initialization guard. Those
behaviors were preserved rather than covered with defensive branches that
would change the original ABI or control flow.

The measured TimeBar interval raises the whole-binary score from 44.9592%
to 44.9746%: six functions improve and `NuTimeBarCreateSet` becomes exact.
One unrelated `NuThreadInitPS` fuzzy score shifts from 99.98% to 99.74%
because the original `"Main"` allocator literal changes linked PIC
displacements; its instruction sequence and control flow are unchanged.
The faithful allocator is retained rather than altering unrelated source
to chase that link-layout artifact. Target/WASM/native builds, lint, four
repository checks, and a 120-frame Cantina smoke test pass.

### Android debris renderer owner

All three reconstructed debris-buffer functions embed the original
`nu3d/android/nuptl_android.c` source path in their GL critical-section
calls. The original local-symbol block ends in
`_GLOBAL__sub_I_nuptl_android.c` and contains its own six `VuVec` objects,
GL buffer state, and local vertex-attribute helpers. Renaming the current
`nuptl_flush.cpp` implementation to that original `.c` basename, compiling
it as C++ as its mangled functions and initializer require, and restoring
the local vector constructors makes the named initializer exact. The
three existing buffer-function scores do not regress.

The original TU is larger than those three functions. It also contains a
second set of TU-local vertex-attribute helpers around `0x296ba4` and the
debris display-list callback at `0x29848f`; these are distinct from the
near-exact `0x293xxx` geometry helpers in `nuiosdl_gl.cpp`. Moving the
callback with its own three ordinary bind helpers into `nuptl_android.c`
restores that ownership without changing the geometry renderer's copies.
The callback and the three buffer functions are the only consumers of the
debris GL-buffer and system-memory arrays and read-buffer index, so these
three globals can now have their original file-local linkage. Their
declarations reach callers through `nuptl_android.h`; the remaining renderer
state is still shared with functions in other current files. Those broader
ownership edges need their own measured migration, not substitute getters
invented for matching.

The measured combined TimeBar and debris-renderer interval raises the
whole-binary score from 44.9592% to 44.9873%. The debris callback improves
from 59.61% to 83.26%, its three original-local bind helpers rise from zero
to 85–88%, the buffer functions gain about six or seven percentage points
each, and the original named initializer becomes exact. The one unrelated
`NuThreadInitPS` PIC-displacement shift described above remains; no debris
function regresses. These gains come from correcting the actual source and
local ownership, not altering ABI or optimizer behavior for a score.
Target, WASM, and native builds, all three lint variants, the four repository
checks, complete symbol coverage (13,425/13,425), and a 120-frame Cantina
fixture smoke test pass after the callback and header migration.

The next contiguous setup run in the same original unit contains
`CreateDmaParticleSet`, `CreateDmaParticleSetGlass`,
`CreateDmaPartEffectList`, and `LinkDmaParticalSets` at
`0x2985ab`–`0x298749`. These four unchanged bodies moved from the generic
render catch-all into `nuptl_android.c` in original address order. Their
actual callers now include `nuptl_android.h` instead of keeping private
link-time declarations. All four match scores—including the two already
exact functions—and the whole-binary 44.9873% score are unchanged. The move
corrects source ownership and the header boundary without disguising the
two bodies that still need code reconstruction.

The preceding particle packet run is also in this owner: the original
addresses place `NuRndrSetParticleRotation` at `0x297248`,
`NuRndrParticleGroup` at `0x297262`, `BuildDebrisVerts` at `0x29765c`, and
`AddParticleGroupToDisplayList` at `0x298394`, before the debris callback.
The current definitions were dispersed among the generic renderer, a support
stub file, and the gameplay particle file despite sharing this TU's debris
matrix, camera plane, particle packet, and vertex-buffer state. Their
unchanged bodies now reside in original order in `nuptl_android.c`, with
actual cross-unit calls declared in `nuptl_android.h` and the remaining
`NuRndrParticleSetRepeat` declaration in its real renderer header.

Moving the rotation setter alone raises its score from 64.75% to 77.25%.
Moving the packet path then raises `NuRndrParticleGroup` from 27.74% to
78.43% and `AddParticleGroupToDisplayList` from 9.93% to 34.77%; the vertex
builder and callback retain their scores. Overall matching reaches
44.9990% from the preceding 44.9873%, without an exact-match loss. Two
unmodified functions remaining in `nurndr_plain.cpp` have small fuzzy-score
declines (`NuRndrCircle` 9.76% to 8.11%, `NuRndrSphereMtx` 93.16% to
92.86%). Their byte-level cause is not proven; the structurally evidenced,
net-positive move is retained without claiming they are relocation-only.
The final source passes target, WASM, and native builds, all three lint
variants, the four repository checks, symbol coverage (13,425/13,425),
and repeated 120-frame Map/Cantina fixture smokes. One rebuilt smoke attempt
did stop in the existing `MovePlayer` → `NuAtan2D` path on a `nutrig.cpp:77`
UBSan table-index error; three subsequent rebuilt runs loaded Map and
advanced all 120 frames. The intermittent sanitizer failure is recorded as
an unresolved runtime observation, not claimed to be caused or fixed by
this TU move.

### Remaining particle-renderer run

The original initializer-delimited `nuptl_android.c` local block starts
with its `VuVecSet` at `0x296ad4`, owns file-local debris matrix, camera
plane, particle packet, and vertex-buffer state, and ends at
`_GLOBAL__sub_I_nuptl_android.c` at `0x29a9d5`; the next block starts with
another `VuVecSet` at `0x29a9f4`. The map correctly cautions that an
initializer-delimited block may contain another unit, so source-path and
shared-static evidence remain important. Immediately after the DMA setup
run are `NuRndrParticleSetRepeat` at `0x2987c6`, `NuRndrParticleDraw` at
`0x2989d8`, and `GenericDebinfoDmaTypeUpdate` at `0x299204`.

Moving the unchanged Repeat body from `nurndr.cpp` into the original particle
owner raises it from 22.18% to 89.55%, with no other function changes.
Moving the unchanged Draw body next raises it from 13.18% to 71.90%, again
with no other score changes. The Repeat declaration now belongs in the
particle owner header, and dead source-local debris declarations were
removed from the previous owners.

`GenericDebinfoDmaTypeUpdate` at `0x299204` then moved unchanged from the
render catch-all into the same original run. It stayed at 40.02%, showing
that owner correction alone could not match its large body. Its three
private curve/colour helper functions were reconstruction artifacts: the
original 5,724-byte body has no helper calls beyond its PIC thunk and
`NuStrCmp`. The source now performs five ordinary inline key searches per
frame (width, height, rotation, RGB, alpha), with inclusive key intervals
and elapsed-zero interpolation branches, matching the disassembly. It also
removes an entry null guard absent from the original, preserves 32-bit
signed/unsigned colour conversion without signed-shift UB, and reuses the
rounded texture-offset numerators for extent coordinates. These changes
raise the body from 40.02% to 60.06%; the whole-binary report reaches
45.0569% from the preceding commit's 44.9990%, with no regressions or exact
losses. Target/WASM/native builds, all three lint modes, four repository
checks, complete symbol coverage, and a rebuilt 120-frame Cantina fixture
smoke pass on the final source.

Once the particle/Draw/update consumers had migrated, the original
`_ZL...` debris matrix, camera plane, packet, buffer counts and pointers,
and write index had no remaining cross-TU source users. Giving their
definitions file-local linkage restores the original data-symbol ownership.
Seven functions improve and none regress: `NuDebrisRendererNextBuffer`
59.86% to 96.13%, Flush 57.43% to 93.06%, Rotation 77.25% to 99.88%,
Init 75.61% to 95.23%, Group 78.43% to 94.32%, Build 52.60% to 58.01%,
and Draw 71.90% to 75.42%. Overall matching rises again to 45.0737%.
Ordering the declarations to mirror the original local BSS symbol sequence
changes no function score, but the rebuilt object's BSS order now matches
that sequence. No fake accessors or export aliases were introduced. The
frozen-source gate passed target, WASM, and native builds, all three lint
modes, four checks, 13,425/13,425 symbol coverage, and a rebuilt 120-frame
Cantina fixture smoke.

The original local block also contains its own display-list helper chain:
`NuDisplayListSetNext` at `0x296b2f`, `NuDisplayListSetID_CALL` at
`0x296b3e`, and `NuDisplayListAddItem` at `0x296b4b`. The original
`AddParticleGroupToDisplayList` calls that last helper directly after
linking its list items. Reconstructing these as ordinary file-local
functions, rather than manually writing the item fields in the caller,
makes the first two helpers exact and raises `NuDisplayListAddItem` to
83.10%. The caller improves from 34.77% to 54.86%. Its original code
unconditionally dereferences the display list; removing a provisional
null guard restores that behavior and accounts for the final caller gain.
The whole-binary report reaches 45.0766%, with seven improved functions,
none regressed, and two new exact matches relative to the preceding
data-order trial. The other TUs' independent copies of these static
helpers remain untouched. Target/WASM/native builds, all three lint modes,
four checks, and 13,425/13,425 symbol coverage pass on the final source.
The rebuilt Cantina smoke loaded the area and reached gameplay on each
attempt, but two attempts stopped in the previously observed
`MovePlayer` → `NuAtan2D` trig-table UBSan failure; two attempts advanced
all 120 frames. The likely mechanism is a nonzero AI movement vector with
squared length at most `1e-6`: `NuFsqrt` returns zero in that interval,
while `MovePlayer` only checks for an exactly zero vector before taking
the reciprocal. The original `NuFsqrt` disassembly at `0x292ec5` has the
same threshold path, and its `MovePlayer` disassembly at `0x102129`–
`0x102193` similarly divides without a rounded-zero guard. This appears
to be a source-fidelity/runtime tension rather than a direct debris-path
regression. It is not treated as a clean runtime pass or hidden with a
sanitizer suppression.

### Display-list Android helper chain

The next original local block names `nudlist_android.c` and contains a
separate setter chain at `0x29aaa9`–`0x29ab92`. Its `NuDisplayListSetItem`
stores the type and calls `NuDisplayListSetNext` before dispatching the
item ID to one of the CALL, CNT, NEXT, or RET setters. Five immediately
following public `NuDisplayListAdd*` wrappers call `SetItem`. These
functions had been split between generic display-list stubs and
`nucore_plain.cpp`, where several `__used__` definitions were no-op
symbol placeholders. A new `nudlist_android.c`, compiled as C++ at the
original unoptimized level, holds the real file-local call chain and
five wrappers; the placeholders are removed rather than preserved for
symbol counts.

The isolated trial raises whole-binary fuzzy matching from 45.0766% to
45.0808%. `SetNext`, CALL, CNT, NEXT, RET, and `SetItem` become exact;
`SetID` reaches 99.58% with only branch-target address differences. All
five wrappers remain exact, with seven improved functions, no regressions,
and six additional exact matches. Original TU-local copies of `SetNext`
and CALL also exist in `nuptl_android.c` and `nuprim_android.c`; the
former remains exact and the latter awaits its own natural caller-based
reconstruction. Target/WASM/native builds, all three lint modes, all
four repository checks, 13,425/13,425 symbol coverage, and a rebuilt
120-frame Map/Cantina smoke pass on this source.

The next low-scoring contiguous run in that owner is
`NuDisplayListLinkItem` at `0x29ad12`, `NuDisplayListLinkItemVP` at
`0x29ad5b`, and `NuDisplayListLinkItems` at `0x29ae31`. Their old
`nucore_plain.cpp` `-O3` definitions wrote item IDs directly and used
a differently named local setter; the original unoptimized Android TU
calls its own static CALL/NEXT/SetNext helpers and `GetBuffer`. Moving
the three together preserves that real call graph. The existing
`nudlist.h` static `GetBuffer` emits the required per-TU copy naturally;
no duplicate definition or forced emission is needed.

All three now match exactly: 16.26%, 8.64%, and 32.13% respectively
become 100%. The isolated whole-binary score rises 45.0953% to
45.1028%, with exactly three improvements, three new exact matches,
and no regressions. Target/WASM/native builds, three lint modes, four
checks, and 13,425/13,425 symbol coverage pass. The rebuilt Cantina
smoke hit the documented original trig UBSan failure on its first
attempt, then passed 120 frames on its second; the flake remains
visible rather than suppressed.

`DisplayListSetAlphaPS` at `0x29b8c0` is another body in the same
original Android TU. Its original 92-byte code clamps alpha to [0, 1]
with ordered comparisons, leaving NaN unchanged, and stores it in the
previous item's matrix `m33`. Moving the source-level operation from
`nudlist.cpp` to this `-O0` owner raises the body score from 13.04% to
99.85%; the remaining four objdiff differences are constant GOT
displacements. The isolated whole-binary score rises 45.1028% to
45.1045% with no other changed function scores. Target/WASM/native
builds, three lint modes, four checks, complete symbol coverage, and a
rebuilt 120-healthy-frame Map smoke pass.

The next lower-scoring body, `NuDisplayListPrepareFaceonPS` at
`0x29b797`, is a 19-byte original function that returns its face-on
argument after assigning it to a stack local. Moving the natural
source-level operation to the same `-O0` TU raises its score from
1.75% to 99.25%, with one remaining load-address instruction
difference. An otherwise unused local solely to match that instruction
was not added. The whole-binary score rises 45.1045% to 45.1049%, with
no other score changes. A real `nudlist.h` declaration replaces the
source-local declaration; target/WASM/native builds, all lint modes,
four checks, full symbol coverage, and the rebuilt 120-frame Map smoke
pass.

The same original display-list block has a local
`NuDisplayListResetBuffer` at `0x29aa49`, called by its
`NuDisplayListInit` at `0x29ab93`; a distinct local copy at `0x295fa9`
is called by `NuRndrSwapScreen` at `0x2967db` in original
`nurndr_android.c`. There is no original
exported ResetBuffer symbol. Today one public definition in
`nudlist.cpp` serves both displaced callers. Restoring this boundary
requires moving each caller with its own naturally emitted local copy
before deleting that public definition; an exported alias would only
hide the incorrect call graph. This is a later two-TU unit, not folded
into the already-exact link triplet.

### Primitive Android begin cluster

The next original local block identifies `nuprim_android.c`. Its text run
places `NuPrimPushCoordSystem` at `0x29cbc9`, a local
`NuDisplayListGet2dList`, then separate static `SetNext`, `SetID_CALL`,
and `AddItem` helpers before `NuPrim2DBegin` at `0x29ccc5` and
`NuPrim3DBegin` at `0x29ceb8`. Both begins call that same `AddItem`, so
their helper copy can be emitted by real source calls. The previous
implementations were split between `nurndr_plain.cpp` at `-O3` and
`nuprim.cpp`, while `nucore_plain.cpp` contained an unused `__used__`
`AddItem` placeholder. A new C++-mode, unoptimized `nuprim_android.c`
now owns the begin chain; the exact `NuPrimPushCoordSystem` and its
`NuPrimInit` caller moved with it. The unused placeholder and an
incorrectly exported `NuDisplayListGet2dList` duplicate were removed.
The shared primitive stream pointer and vertex count remain global
because real callers in other TUs use them.

This isolated unit raises fuzzy matching from 45.0808% to 45.0880%:
`NuPrim2DBegin` improves from 17.51% to 71.57%, `Get2dList`,
`SetNext`, and `SetID_CALL` become exact, and the already exact
`NuPrimPushCoordSystem` and `NuPrimInit` stay exact. Seven functions
improve, four new exact matches appear, and one untouched function,
`NuRndrLineStrip2di`, slips from 42.08% to 42.00%. Inspection found
a real register-allocation/code-generation difference after the moved
`NuPrim2DBegin` call; it is not asserted to be a relocation-only change.
The structurally evidenced, net-positive unit is retained with that
small regression recorded. Target/WASM/native builds, all three lint
modes, four repository checks, 13,425/13,425 symbol coverage, and a
rebuilt 120-frame Map/Cantina smoke pass.

The contiguous continuation of this original TU has `NuPrim2DEnd` at
`0x29d0c9`, `NuPrim3DEnd` at `0x29d0f6`, `NuPrim2DAddXYZ` at
`0x29d123`, then `NuPrimSetCoordinateSystem` at `0x29d3d1`. Moving the
End pair and vertex writer from `nurndr_plain.cpp` into
`nuprim_android.c` restores their common owner and lets their two
exclusive bookkeeping variables become genuine file-local data. The
original `g_NuPrim_VertexCountPtr` is zero-initialized BSS, while
`g_NuPrim_CurrentPrimType` is a 16-bit `.data` value initialized to
`10000`; the latter was previously zero-initialized under a different
name. The vertex writer shares a real 0x18-byte internal layout header
with remaining renderer consumers, with its quad-copy order preserved.
`g_NuPrim_StreamBufferPtr` and `g_NuPrim_VertexCount` remain exported
because callers in other TUs use them. Moving near-exact
`NuPrimSetCoordinateSystem` in original order leaves its score unchanged.

The staged measurement rises from 45.0880% to 45.0953% without further
function regressions. Both Ends rise from 74.08% to 99.92%,
`NuPrim2DAddXYZ` from 27.11% to 71.34%, `NuPrim2DBegin` from
71.57% to 77.30%, and `NuPrim3DBegin` from 72.54% to 74.88%. No
attribute or forced-emission stand-in was added for the static data.
Target/WASM/native builds, all three lint modes, four checks,
13,425/13,425 symbol coverage, and build-file formatting checks pass.
On the rebuilt Cantina fixture, two 120-frame attempts reached gameplay
then hit the previously documented original trig-table UBSan failure;
the third reached 120 healthy frames. This is a flaky smoke result,
not a clean pass. The original behavior is deliberately preserved for
matching, as requested by the user; no sanitizer suppression was added.

The original display-list bootstrap has its own local `NuDisplayListResetBuffer`
at `0x29aa49`; `NuDisplayListInit` at `0x29ab93` calls that copy directly.
The separate renderer copy at `0x295fa9` remains to be reconstructed with
`NuRndrSwapScreen`. Moving the initializer from `nucore_plain.cpp` into the
original O0 `nudlist_android.c` restores this ownership without an alias or
forced symbol. The original `NuInitHardware` call passes the stream end by
value, so `NuDisplayListInit` now takes `VARIPTR` rather than a pointer to it;
the initializer uses the recovered manager fields and a real header
declaration. The existing global reset helper stays temporarily for the
renderer caller until that second TU is moved.

Against `/tmp/nudlist_faceon_final.json`, the staged report rises from
45.104927% to 45.106697% fuzzy matching. `NuDisplayListInit` rises from
49.565216% to an exact 100% at the original 90-byte size; the only other
whole-report change is a negligible improvement in `NuInitHardware`, with no
function regressions. Target/WASM/native builds, all three lint modes, four
repository checks, and 13,425/13,425 original text-symbol coverage pass.
The rebuilt 120-frame Map/Cantina fixture smoke passed on this attempt; the
previously documented original movement/trig sanitizer flake remains possible.

The renderer owns a second, separate local `NuDisplayListResetBuffer` at
`0x295fa9`, followed by a local empty `NuDisplayListCheckBuffer` at
`0x295fd7`. Original `NuRndrSwapScreen` calls both directly, while its
`NuRndrSwapScreenEx` wrapper immediately follows it. These two exported
functions and their private helpers now live together in the original O0
`nurndr_android.c` unit; its real C++ dependencies require compiling the
`.c`-named file as C++ while keeping the existing C ABI on its exports.
After the move, the temporary global reset implementation and separate
host-only check stub were removed. The build has exactly two local reset
copies and no global reset, matching the original symbol boundary. The
existing host weak behavior of `NuRndrSwapScreen` was retained; no new
attribute, alias, or forced-emission helper was introduced.

Against `/tmp/nudlist_init_stage1_final.json`, fuzzy matching rises from
45.106697% to 45.109860%. `NuRndrSwapScreen` improves from 46.97619% to
an exact 100% at the original 173-byte size, and `NuRndrSwapScreenEx`
from 54.294117% to an exact 100% at 48 bytes. No other function score
changed. Both private reset copies are 48 bytes in the current build
versus 46 in the original due to ordinary expression code generation;
their source behavior is preserved. Target/WASM/native builds, all three
lint modes, four repository checks, and 13,425/13,425 original text-symbol
coverage pass. The rebuilt 120-frame Map/Cantina fixture smoke passed on
this attempt; the original movement/trig sanitizer flake remains documented.

### Static display-list dispatch tables

The original `nudlist_android.c` owns a writable, file-local 49-entry
`__ItemFnTable` at `0x625ae0`, a writable global 49-entry
`__ShadowItemTable` at `0x625bc0`, and file-local `CurrentItemTable` at
`0x625c84`. The latter is initialized to the primary table by a data
relocation. These are indexed by item type minus `0x80`, as confirmed by
`NuDisplayListExecute`. The prior implementation instead populated two
256-entry arrays in BSS through C++ constructors and selected an interior
pointer. The original table shapes and static initialization now live in
the original O0 display-list unit alongside `NuDisplayListDrawItems` and
`NuDisplayListSetItemTable`; the executor remains in its separate owner.
A focused internal callback header declares the functions used by the
tables and their definition units. The host-only item histogram no longer
prints a handler pointer, removing its sole dependency on the non-original
public 256-entry table.

Against `/tmp/nurndr_reset_stage2_final.json`, fuzzy matching rises from
45.109860% to 45.110252%. `NuDisplayListDrawItems` improves from 53.29% to
83.24% and `NuDisplayListSetItemTable` from 49.89% to 54.33%, with no
function regressions. The target ELF has local/global/local data symbols
of 196/196/4 bytes, respectively; the current-table word has an
`R_386_RELATIVE` relocation to the primary table base, and the two old
table-constructor symbols are absent. Target/WASM/native builds, all three
lint modes, four repository checks, and 13,425/13,425 original text-symbol
coverage pass. The rebuilt Map fixture reached 120 healthy frames.

A further ordinary source-shape trial changes `NuDisplayListSetItemTable`
from two direct comparisons to a two-case `switch`, reflecting the
original's single load/test/branch sequence. This raises that body from
54.33% to 94.06% and the whole score from 45.110252% to 45.110740%,
with no other function-score changes. The remaining `DrawItems` gap
looks like an otherwise-unused O0 local copy of its argument; no
speculative local was added merely to gain matching.

### Graphics clear boundary correction

The checkpoint first placed `Nu360_dxClear` at `0x317070` in a new
`nuscratch_android.c` file because it follows the scratch allocator
run. A fuller original local-symbol audit corrects that attribution:
`_GLOBAL__sub_I_nuscratch_android.c` ends the scratch object, then the
clear-local `_ZZ13Nu360_dxClearE10lastColour` and
`_GLOBAL__sub_I_ios_graphics.cpp` belong to the following object.
The clear function directly precedes its optimized framebuffer
functions at `0x317190`–`0x3173c0`. The original scratch allocators are
unoptimized in their own TU; the clear is not a mixed-optimization
member of that TU. No function-level optimization attribute or pragma
is warranted.

The correction moves the clear wrapper and its four-byte function-local
BSS cache into the existing `ios_graphics.cpp` owner, declares its API
in `ios_graphics.h`, and removes the mistakenly named source file and
its build/lint overrides. Against the 45.123135% callback baseline, the
target score rises to 45.123142%: `Nu360_dxClear` remains 69.208954%,
`NuIOS_AllocateSystemFramebuffers` gains 90.00→90.12%, and
`NuIOSInitOpenGLES` becomes exact from 99.74%, with no regressions.
The scratch allocator source and data were subsequently moved into their
own O0 TU, as recorded in the next section.
Target/WASM/native builds, all three lint modes, four repository checks,
13,425/13,425 text-symbol coverage, and the rebuilt 120-healthy-frame
Map smoke pass after the correction.

### Original Android scratch allocator unit

The original address run `0x316d41`–`0x31706c` is an O0 C++ translation
unit named `nuscratch_android.c`. Its five public allocator bodies are
`NuScratchReset`, `NuScratchAlloc32`, `NuScratchAlloc64`,
`NuScratchAlloc128`, and `NuScratchRelease`, in that order. The same unit
owns the global `PS2_SCRATCH_BASE` and local `ps2_scratch_free` BSS. The
original three allocators each repeat the alignment, size-rounding, and
last-pointer push sequence; there is no shared allocator helper in the
original text. The original TU also emits a VuVec static initializer and
`_GLOBAL__sub_I_nuscratch_android.c` from its `nuvuvec.hpp` inclusion.
The declarations follow the original BSS order: backing array, six VuVec
locals, then the private cursor.

These bodies and data now live in that real source unit, with `numem.h`
declaring all five exports and the removed helper no longer compiled from
`nucore_plain.cpp`. Against the 45.123142% immediately preceding report,
the target score reaches 45.129750%. `NuScratchReset` rises 54.88→99.88%,
Alloc32 25.03→99.79%, Alloc64 and Alloc128 25.00→99.79%, and Release
53.80→83.80%. The constructor is exact; no other function regresses.
The remaining Release mismatch is a genuine compiler expression-form
difference (`lea` plus dereference versus direct indexed load), not a
reason to introduce a matching-only construct. Clear remains in its
separate, following `ios_graphics.cpp` TU.

Target, WASM, and native/smoke builds pass, as do target/native/WASM lint,
all four repository checks, and the exact extra-symbol baseline. Original
text-symbol coverage remains 13,425/13,425 with zero missing. The rebuilt
Map/Cantina fixture advanced 120 healthy frames on this attempt; the
original movement/trig sanitizer flake remains documented above.

### Android display-list callback continuation

Six callbacks in the original `nuiosdl_gl.cpp` address run were still
defined in the displaced `nu2api_nucore_misc.cpp` owner. The original
addresses are `NuIOSDLSkinMtxCallback` at `0x294764`,
`NuIOSDLVertexOffsetsCallback` at `0x294d93`, `NuIOSDLFogCallback` at
`0x2951b0`, `NuIOSDLLightmapOld` at `0x295420`,
`NuIOSDLLightmapOffsetOld` at `0x2954f0`, and `NuIOSDLLightmap` at
`0x2955ee`. They now live in `nuiosdl_gl.cpp` under its existing `-O2`
optimization setting, using the actual display-list callback, texture,
fog, and shader headers. The skin callback is an ordinary definition,
without the displaced weak attribute. Packet interpretation and GL
state operations were preserved.

The isolated whole-binary score rises from 45.110740% to 45.123135%.
In that order, the six callback scores rise 26.94→82.58%,
35.97→71.16%, 24.64→60.24%, 30.88→70.47%, 27.12→77.03%, and
23.95→58.70%. Three untouched functions in the displaced miscellaneous
file show small score shifts: `NuHGobjEvalAnimBlend2Root_3` improves
88.92→89.10%, while `NuDisplayListCreate` changes 53.25→53.15% and
`NuIOS_CreateGLTexFromPVRInMemory` 26.44→26.36%. Those two losses are
not called relocation-only: deleting definitions from a GCC source file
can alter generated code and section placement of its remaining bodies,
and their own source was not changed. The original address run and
aggregate gain justify retaining this coherent six-function unit, with
the two small secondary regressions recorded rather than hidden.

Target, WASM, and native smoke builds pass, as do the target/native/WASM
lint modes and all four repository checks. Original text-symbol
coverage remains 13,425/13,425, with zero missing. A rebuilt Map/Cantina
fixture smoke passed 120 healthy frames on this attempt; the original
`MovePlayer`/`NuFsqrt` trig sanitizer flake remains documented elsewhere.

### Display-scene unclip and scene type boundary

The original ordinary-text run contains `NuDisplaySceneUnclip` at
`0x2f1180` (112 bytes), followed without intervening local symbols by
`InvalidateClipRanges`, `NuDisplayListExecute`, `DefaultMtl`,
`NuMtlUpdate`, and `NuDisplaySceneClone`. This supports the existing O2
display-list core owner in `nudlist.cpp`; the exact original filename is
still unproven. The former empty stub lived in the unrelated catch-all
`nu2api_nucore_misc.cpp`.

The original C++ symbol uses `nudisplayscene_s` for the 0x90-byte
display-list scene. The current tree had assigned that tag to a separate
0x218-byte render-parameter structure. The two types are now named
coherently: `nudisplayscene_s` for the display-list scene and
`nurenderscene_s` for the render parameters. Related declarations and
callers use the real header types, not a cast or linker-only declaration.
Unclip clears the first item ID of every scene material, then marks each
clip object present in the active two-bit clip bitmap. The original
`_Z20NuDisplaySceneUnclipP16nudisplayscene_s` name is emitted from the
ordinary C++ declaration, and the current body is 112 bytes, the same
size as the original.

The shared target trial in `/tmp/unclip_scene_trial.json` raises Unclip
from 9.767442% to 96.279070%; it also includes simultaneous quadtree
work, so its 45.171870% aggregate is not attributed to this function
alone. `PreWarmGeomsAndBakeVAOs` improves 51.94→54.66% from the type
boundary correction. The untouched `NuDisplayListCaptureSortPriority`
changes 68.83→68.61% with unchanged 249-byte size; objdiff shows its
existing loop/control-flow differences, and this small secondary shift is
recorded as layout/matcher churn, not hidden. The remaining two Unclip
instruction differences are the original's unsigned-byte active-buffer
index versus the compiler's signed shift; both select the same 0/1 index
for the existing field values. A source trial explicitly taking the low
byte produced the same score, so the simpler expression was retained.
The target build/report passed, and the combined NuQT gate on this source
passed target/WASM/native builds, all three lint modes, four checks, full
13,425/13,425 text-symbol coverage, and a rebuilt 120-frame Map/Cantina
smoke. The original movement/trig sanitizer flake remains documented.

### Quadtree insertion ownership

The original `nuqt.cpp` run contains local `InsertData` at `0x2617f7`,
local recursive `AddElementR` at `0x261a8d`, and exported
`NuQTAddElement` at `0x2622e3`. These now live together in the O0
`nuqt.cpp` owner, with the public six-argument declaration in `nuqt.h`.
The empty public definition in `nucore_plain.cpp` and the two unrelated
`__used__` local placeholders in `rtl.cpp` and `gamelib_ogg.cpp` were
removed. The original's unusual behaviors are retained: `InsertData`
uses an inverted initial-capacity comparison and a cached null destination
for `memcpy`, while redistribution tests the new item's bounds for old
items and compacts their data after recursion. No safety correction or
forced symbol emission was added.

Against `/tmp/scratch_final.json` at 45.129750%, the isolated whole-binary
fuzzy score reaches 45.160545% with no unrelated function regressions.
`InsertData` scores 99.93%, and `NuQTAddElement` 92.98%. A subsequent
source-faithful unrolling of the original four explicit quadrant branches
raises `AddElementR` from 46.13% to 66.56% in the shared target report;
this run also includes the independently reconstructed display-scene
callback, so its aggregate is not attributed solely to quadtree work.
The remaining recursive-body mismatch reflects source shape/codegen still
to be reconstructed, not a reason to add assembly or matching-only
attributes. Target, WASM, and native/smoke builds pass, as do all three
lint modes, all four repository checks, and the extra-symbol baseline.
Original text-symbol coverage is 13,425/13,425 with zero missing. The
rebuilt Map/Cantina fixture passed 120 healthy frames on this attempt;
the independently documented original movement/trig sanitizer flake
remains possible on other runs.

### Android material plane and copy ownership

The original `numtl_android.cpp` ordinary-text run places `NuMtlInsert`
at `0x29c090` (25 bytes), `NuMtlSetRenderPlane` at `0x29c0b0`
(77 bytes), and `NuMtlCopy` at `0x29ca00` (46 bytes), immediately before
`NuMtlInitExPS` at `0x29ca30`. The adjacent Android-specific rendering
functions and original material-unit source path support this ownership.
The two plane functions write bits 4..11 of the material's first word;
SetRenderPlane then calls Insert. Copy transfers the full 0x2c4-byte
material while preserving the destination display-list pointer at +0x3c.
These are normal typed definitions in the existing `-O2` material unit,
declared in `numtl.h`; the misplaced empty stubs were removed.

The pre-material `/tmp/scratch_final.json` report scored 45.129750%:
Insert 35%, SetRenderPlane 20%, Copy 30%. Parallel NuQT/display-scene
work produced `/tmp/unclip_scene_trial.json` at 45.171870%; this is a
**partial material snapshot**, already containing exact Insert at 100%,
while SetRenderPlane and Copy remained 20% and 30%. The final report
scores 45.174390%, with all three bodies at 100% and their original
25/77/46-byte sizes. Address-keyed comparison against the partial shared
report changes only SetRenderPlane (20→100%) and Copy (30→100%); no
scored body regresses. Assigned exact-body counts increase 4,471→4,474
(the report-wide matched-function measure is 4,705→4,708), including
Insert newly assigned at 100% where the partial report did not count it.

Target, WASM, and native/smoke builds pass, as do target/native/WASM lint,
all four repository checks, and the extra-symbol baseline. Original
text-symbol coverage is 13,425/13,425 with zero missing. The rebuilt
Map/Cantina fixture passed 120 healthy frames on this attempt; the
original movement/trig sanitizer flake remains documented above.

### Display-list clip-range and executor continuation

The original display-list core sequence directly joins
`NuDisplaySceneUnclip` at `0x2f1180`, `NuInvalidateClipRanges` at
`0x2f11f0`, and `NuDisplayListExecute` at `0x2f1230`. The already-exact
clip-range reset still lived in the catch-all `nucore_plain.cpp`, despite
using the real 0x90-byte display-list scene and falling between the two
functions owned by `nudlist.cpp`. Moving its unchanged body into that core
owner, with its exported declaration in `nudlist.h`, leaves the target
whole-binary score at 45.174390%, the body exact, and every other score
unchanged in `/tmp/invalidate_move.json`.

The executor's original 78-byte control flow distinguishes NEXT (follow
the link), CNT (advance 16 bytes), CALL (dispatch a non-null handler by
type), and all other IDs (return). The old nested loop expressed this
behavior but scored 26.77%. Expressing the same four outcomes as one
ordinary switch causes GCC to emit the original case ordering and raises
the body to 82.07% in `/tmp/execute_full_switch_trial.json`. The overall
score reaches 45.175297%, with no other score changes or exact loss. The
remaining difference is chiefly register allocation for the handler table
and the associated prologue; no dummy use or attribute was added to force
it. Target/WASM/native builds, all three lint modes, four repository
checks, 13,425/13,425 original text-symbol coverage, and the rebuilt
120-frame Map/Cantina smoke pass on this source.

### Android material callback control flow

The original `NuIOSDLMtlCallback(void*)` at `0x29c480` is a 1,321-byte
member of the `numtl_android.cpp` material run. It selects the debris,
face-on, or ordinary shader path, binds the corresponding vertex format
and textures, uploads four debris shader constants, and finishes with
the material Z and render states. Its former source already implemented
those behaviors, but nested the ordinary/face-on path first even though
the original compiler lays out debris as the fall-through path. The
four parameter searches also manually shifted a packed word; the real
`NUSHADERPROGRAMPARAMETER` type provides the 12-bit location and 4-bit
setter fields directly. Using those fields and restoring debris-first
branch order preserves the behavior and follows the original body shape.

Against the frozen pre-callback `/tmp/execute_full_switch_trial.json`
report (45.175297%), the address-keyed callback rises 13.371795→
78.464745%; the final whole-binary score is 45.193504%. Its current
body is 1,303 bytes, 18 shorter than the original, with remaining
register-allocation and parameter-loop code-generation differences.
Every other scored address is unchanged and the exact-match count
remains 4,474. Target, WASM, and native/smoke builds pass, as do all
three lint modes, four repository checks, and the extra-symbol baseline.
Original text-symbol coverage is 13,425/13,425 with zero missing. The
rebuilt Map/Cantina fixture advanced 120 healthy frames on this run;
the original movement/trig sanitizer flake remains documented above.

### Display-scene add and core optimization calibration

`NuDisplaySceneAdd` at `0x2e9e30` (416 bytes) is part of the display-scene
run, alongside Destroy at `0x2e9fd0`. Its original code checks the
sort-priority count before loading the manager's list/count, inserts each
priority into ascending order, then writes the updated manager fields
after the positive-count loop. The previous source wrote the count in
each iteration and wrote the list even for an empty scene. Deferring
those writes preserves the resulting state and raises Add from 58.669567%
to 59.173912% in `/tmp/scene_add_deferred_writes.json`; the whole score
moves 45.193504→45.193546%, with no other scored-body change. The two
source comments mislabeling Add/Destroy as `0x2f9e30`/`0x2f9fd0` were
corrected to their actual `0x2e...` addresses.

The original binary also contains two collected
`DisplayListBeforeFrame(...).constprop` clones, which makes the core TU's
optimization level worth testing. A whole-file `nudlist.cpp` `-O3` trial
raised eight other functions and the aggregate to 45.201054%, but
regressed Add sharply from 59.173912% to 33.017390%. Repeating the trial
with Add's former source still gave 32.356520%, so the loss was not caused
by the deferred-write edit. The exact original file boundary/optimization
is not proven by the collected clone addresses, and a net-positive total
does not justify silently keeping this large individual regression. The
file remains at its calibrated `-O2`; the final restored report exactly
reproduces 45.193546% and all per-function scores. Target/WASM/native
builds, all three lint modes, four checks, 13,425/13,425 original text
symbols, and the rebuilt 120-frame Map/Cantina smoke pass on this source.

### Render-scene callback reload

The original `NuDisplayListDrawRenderScene` at `0x2ed1a0` is 168 bytes.
It caches the initial sort-priority count but reloads the render-scene
slot after the capture/draw callbacks on every iteration, then draws
the current slot's 2D tail and clears that slot. The former source held
the scene pointer and read its count through the loop; callbacks could
change the slot, so this is a real behavioral as well as code-shape
correction. The normal source now retains the cached count and refreshes
the pointer after each callback pair. Against
`/tmp/render_states_field_trial.json`, its score rises 69.551020→
98.775510% and its emitted size remains the original 168 bytes.

The shared whole score rises 45.193947→45.194190%. The untouched
`NuDisplayListSwapBuffersEndFrame` gains 26.850550→26.883516%, while
`DisplayListLinkDynamicMtls` drops 22.403890→21.241419%. Comparing the
large helper's old/new disassemblies found the same 824 instruction
positions but 12 changed mnemonic/register choices; this secondary loss
is genuine TU-wide codegen churn, not merely a relocated call operand.
No change to that helper or matching-only code was introduced. The
combined frozen-source gate with the following material unit passed
target/WASM/native builds, all three lint modes, four checks, complete
13,425/13,425 text-symbol coverage, and the rebuilt 120-frame
Map/Cantina smoke.

### Android material render-state field reads

The original `NuMtlSetRenderStatesPS` at `0x29c1c0` (697 bytes) tests
the debris flag at material +0x1f2 before inspecting alpha-test bits
at +0x42. The real `NUMTLATTRIB` fields map the bytes: `alpha_test`
is +0x42 bits 4..6, `alpha_ref` is bits 7..14 of the word there,
blend mode is +0x40 low nibble, and cull mode is +0x41 bits 4..5.
The existing GL blend cases, alpha-test results, and cull call agree
with the original disassembly. Its source, however, eagerly cached
`alpha_ref` at function entry. Reading the genuine field at its use
sites restores the original debris-first entry and avoids an early
material load. Post-branch `u8` and promoted `u32` caching trials both
regressed the body and were reverted; no state behavior was changed.

Against `/tmp/scene_add_restored_o2.json` at 45.193546%, the isolated
field-read report `/tmp/render_states_field_trial.json` rises to
45.193947%. The address-keyed body improves 34.021427→36.721428%; all
other 12,332 scored addresses are unchanged, none are added or lost,
and 4,474 exact matches remain. The final combined target report is
45.194190% after the separately documented display-list scene change;
the render-state body retains 36.721428%. The remaining difference is
mostly register allocation and blend-switch/control-flow placement,
not a verified new behavior to invent.

On the frozen combined source, target, WASM, and native/smoke builds,
all three lint modes, four repository checks, and the extra-symbol
baseline pass. Original text-symbol coverage remains 13,425/13,425
with zero missing. The rebuilt Map/Cantina fixture advanced 120
healthy frames on this run; the original movement/trig sanitizer flake
remains documented above.

### Android deferred transform callbacks

The original `nuiosdl_gl.cpp` callback run contains
`NuIOSDLDeferredTransformCallback` at `0x2948ce` and
`NuIOSDLDeferredTransformParamsCallback` at `0x294a37`, each 103 bytes.
The long intervening `NuIOSDLTransformParamsCallback` at `0x294935`
is a different routine with tint, material, Z, shader, and shadow work;
none of that work belongs in the deferred callbacks. The plain deferred
body saves the render-stream matrix words at +0x3c/+0x2c, substitutes
1.0f/0.0f, calls the existing `NuRenderContextSetWorld`, and restores
the saved words. The params form does the same at +0x3c/+0x38 and calls
`NuRenderContextSetWorld_transpose`. These functions now live in original
order beside their non-deferred counterparts in the `-O2` Android
display-list unit; the two empty catch-all stubs were removed. Both
signatures were already declared in `nudlist_callbacks.h`.

Against the frozen `/tmp/draw_render_reload_final.json` baseline at
45.194190%, the final target report reaches 45.197998%. At their
original addresses, both deferred bodies rise 12.727273→99.939390%
and retain the original 103-byte size. Across 12,333 address/name keys,
those are the only two score changes; there are no added or lost keys,
no scored regression, and the 4,474 exact-match count is unchanged.

Target, WASM, and native/smoke builds pass, as do all three lint modes,
four repository checks, and the extra-symbol baseline. Original text
symbol coverage remains 13,425/13,425 with zero missing. The rebuilt
Map/Cantina fixture advanced 120 healthy frames on this run; the
original movement/trig sanitizer flake remains documented above.

### Android material reflection callback

The original `NuIOSDLReflectionCallback(void*)` at `0x29c9b0`
(66 bytes) directly follows the material callback and precedes
`NuMtlCopy` in the original `numtl_android.cpp` `-O2` run. Its packet
contains an `i32` reflection flag. The body writes that flag to
`g_renderingReflection`, then, if the material currently in use is
non-null, calls the existing `NuIOS_SetCullMode` with its typed
`NUMTLATTRIB::cull_mode`. The material pointer is the same shared
render-context value set by `NuIOSDLMtlCallback`. This ordinary body
now lives beside its material neighbors, using the existing exported
callback and GL-state declarations; the empty catch-all stub was
removed.

Against the pre-deferred `/tmp/draw_render_reload_final.json` baseline
at 45.194190%, `/tmp/reflection_final.json` reaches 45.199104%.
The two independently documented deferred callbacks and this
reflection callback are the only three changed scores across the
same 12,333 address/name keys. Reflection rises 21.00→100.00% and
retains the original 66-byte body; there are no other scored changes
or missing keys, and exact matches increase 4,474→4,475. The prior
deferred-only target report was 45.197998%, so this reflection move
accounts for the final 0.001106 percentage-point gain.

On this frozen source, target, WASM, and native/smoke builds pass,
as do all three lint modes, four repository checks, and the
extra-symbol baseline. Original text-symbol coverage remains
13,425/13,425 with zero missing. The rebuilt Map/Cantina fixture
advanced 120 healthy frames on this run; the original movement/trig
sanitizer flake remains documented above.

### Cutscene loaded-address and adjacent callbacks

The original `cutscene.cpp` text run has C++
`NuGCutSceneRemapFocusIdToLocaterNum` at `0x4335d0`, C
`NuGCutSceneLoadAddr` at `0x433910` (113 bytes), and C
`NuGCutSceneDestroy` at `0x433990` (50 bytes), followed by
`NuGCutSceneFixUp`. The LoadAddr body accepts only version >9,
records the supplied loaded size, converts string and animation
relocation deltas to the new address, selects the same flags-&-8
pointer-fixup branch as `NuGCutSceneLoad`, remaps focus-camera
indices, clears the temporary string delta, and returns the scene.
It now uses the real `NuGCutSceneFixPtrs_Title` helper and an exported
`nugcutscene.h` declaration instead of an empty catch-all stub.

The already-implemented Remap was moved unchanged from the
miscellaneous unit to the C++ scope immediately before the cutscene
loader, preserving its mangled linkage and real header declaration.
The exact Destroy body was then moved from `nucore_plain.cpp` between
LoadAddr and FixUp; its exported declaration is now in
`nugcutscene.h` rather than a local declaration in `cutscenes.cpp`.
No tentative Destroy loss was retained.

These were measured as three separate stages. Against the frozen
`/tmp/reflection_final.json` baseline at 45.199104%, LoadAddr raises
its original-address body 5.128205→82.615390% while retaining 113
bytes, for 45.200960% overall in `/tmp/loadaddr_stage1.json`; it is
the sole changed scored address. Remap then rises 75.66→75.86%, with
no other score changes, to 45.200966% in
`/tmp/cutscene_remap_stage2.json`. Finally, Destroy remains 100%
and 50 bytes in `/tmp/cutscene_stage3.json` at 45.200980%. The only
new score change at that stage is untouched `NuKeyToAscii`
99.51613→100%, a downstream TU-wide code-generation effect from
removing the old Destroy definition, not intended Destroy behavior.
All stages retain the same 12,333 address/name keys without regressions;
the exact count increases 4,475→4,476 in the final stage.

The frozen combined source passes target, WASM, and native/smoke builds,
all three lint modes, four repository checks, and the extra-symbol
baseline. Original text-symbol coverage remains 13,425/13,425 with
zero missing. The rebuilt Map/Cantina fixture advanced 120 healthy
frames on this run; the original movement/trig sanitizer flake remains
documented above.

### Cutscene system initialization and locator VFX callbacks

The original `cutscene.cpp` run places `NuGCutSceneSysInit` at
`0x433530` (44 bytes) and C++ `NuGCutSceneSysInitVfx` at `0x433560`
(60 bytes), before the focus remapper. SysInit clears the file-static
background and active cutscene-instance lists before storing
`locatorfns`. SysInitVfx stores its four callback arguments directly,
in Lookup, Trigger, Release, Update order. The original BSS places
Update at `0x1268140`, Release at `0x1268150`, Trigger at `0x1268160`,
and Lookup at `0x1268170`; their current definitions are together in
that declaration order in the cutscene owner.

The real `nugcutscene.h` declarations preserve the original VFX
signature: lookup takes `const char*`; trigger takes `(i32, VuMtx*)`
and returns `i32`; release takes `i32`; update takes `(i32, VuMtx*)`.
The locator dispatcher now builds a true 0x40-byte `VuMtx`, uses its
`NUMTX` member for matrix operations, and passes the wrapper directly
to Trigger/Update. This removes the provisional float-array copy and
16-bit callback argument while preserving the original call ABI.

The stages were measured independently. Against
`/tmp/cutscene_stage3.json` (45.200980%), SysInit rises
67.50→99.75%, retaining 44 bytes, to 45.201280% in
`/tmp/cutscene_sysinit_stage4.json`. The type-only VFX correction
raises `instNuGCutLocatorUpdate` 17.357320→25.401985% (1,665 bytes)
and overall matching to 45.204117% in
`/tmp/cutscene_vfx_type_stage5.json`. Moving the four data definitions
to their original owner changes no scored body in
`/tmp/cutscene_vfx_data_stage6.json`. Implementing the direct setter
then raises `NuGCutSceneSysInitVfx` 28→100%, retaining 60 bytes, and
overall matching to 45.205032% in
`/tmp/cutscene_vfx_setter_stage7.json`. Each stage retains the same
12,333 address/name keys with no scored regressions; exact matches
rise 4,476→4,477 in the final stage.

The frozen combined source passes target, WASM, and native/smoke
builds, all three lint modes, four repository checks, and the
extra-symbol baseline. Original text-symbol coverage remains
13,425/13,425 with zero missing. The rebuilt Map/Cantina fixture
advanced 120 healthy frames; the original movement/trig sanitizer
flake remains documented above and was neither changed nor suppressed.
