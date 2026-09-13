# 04 — Types & ABI: mangling, layout, calling conventions, vtables, thunks

> Agent/reference document. Human workflows are in
> [`CONTRIBUTING.md`](../../CONTRIBUTING.md).

Agent-oriented cheat sheet for matching `res/libTTapp.so` (Android x86, i686).
Everything below was verified with the repo's Bazel-managed Android NDK r8e
GCC 4.7 toolchain (`-fno-exceptions -fno-rtti`, C++11 — the flags in
`.bazelrc` and `src/BUILD.bazel`)
against the original binary via `nm`/`objdump`. When in doubt, re-verify: mangling
only matters when the function/symbol is exported (or vtable-referenced), so the
golden rule is *the binary's mangled name is the spec*.

## 1. Verified mangle table (Itanium C++ ABI, GCC 4.7)

Compiled one function per signature, `nm`'d the `.o`, demangled back with
`i686-linux-android-c++filt`. All verified:

| C++ type                | mangle  | verified symbol (demangled)      |
| ----------------------- | ------- | -------------------------------- |
| `int`                   | `i`     | `_Z5f_inti` → `f_int(int)`       |
| `unsigned int`          | `j`     | `_Z5f_intj`-style `_Z8f_myuintj` |
| `long`                  | `l`     | `_Z6f_longl` → `f_long(long)`    |
| `unsigned long`         | `m`     | `_Z11f_abi_ulongm`               |
| `long long`             | `x`     | `_Z3f_xx` → `f_x(long long)`     |
| `unsigned long long`    | `y`     | `_Z3f_yy`                        |
| `char`                  | `c`     | `_Z3f_cc`                        |
| `signed char`           | `a`     | `_Z3f_aa`                        |
| `unsigned char`         | `h`     | `_Z3f_hh`                        |
| `short`                 | `s`     | `_Z3f_ss`                        |
| `unsigned short`        | `t`     | `_Z3f_tt`                        |
| `bool`                  | `b`     | `_Z3f_bb`                        |
| `float`                 | `f`     | `_Z3f_ff`                        |
| `double`                | `d`     | `_Z3f_dd`                        |
| `void *`                | `Pv`    | `_Z5f_PvPv`                      |
| `int *`                 | `Pi`    | `_Z5f_PiPi`                      |
| `char *`                | `Pc`    | `_Z5f_PcPc`                      |
| `const int *`           | `PKi`   | `_Z5f_PKiPKi`                    |
| `int &`                 | `Ri`    | `_Z5f_RiRi`                      |
| `int[5]` param          | `A5_i`  | `_Z6f_A5_iPi` (decays to `Pi`)   |
| `int(*)(int,double)`    | `PFid`  | `_Z6f_PFidPFiidE`                |
| `float(*)(void)`        | `PFfv`  | `_Z6f_fPvfPFfvE`                 |
| namespace `ns1::ns2`    | `_ZN…`  | `_ZN3ns13ns21fEi`                |
| member `Widget::method` | `_ZN…`  | `_ZN6Widget6methodEii`           |
| struct arg              | by name | `_Z4f_PSP1S` (`S` param → `P1S`) |

### THE TRAP: `int` vs `long` (both 4 bytes, different mangles)

On this target `sizeof(int) == sizeof(long) == 4`, but `int` mangles `_i` and
`long` mangles `_l`. A signature written with the wrong one silently produces a
non-matching symbol. Verified side-by-side:

```
_Z5f_inti     → f_int(int)          // int
_Z6f_longl    → f_long(long)        // long — same width, different code
_Z11f_wrong_intl → f_wrong_int(long) // wrote `int f(long)` by mistake: still _l
```

### typedefs preserve the underlying mangling

`typedef int myint; myint` mangles as `_i` (verified: `_Z7f_myinti`), `typedef
long mylong` as `_l` (`_Z8f_mylongl`). This is exactly why `fixed_width.h`
exists (see §2). Also verified: `size_t` mangles `_j` (it's `unsigned int` on
this target), `ssize_t` mangles `_l` (it's `long`).

## 2. fixed_width.h / common.h — the abi_long trick

`src/nu2api/nucore/fixed_width.h:19-27`:

```c
// ABI-mangling-compatible `long`/`unsigned long` for the 32-bit target.
// `long` mangles as `_l` and `unsigned long` as `_m` in the Itanium C++ ABI,
// and the reconstructed ABI stubs (ogg/vorbis, legoapi) genuinely used these
// in the original binary, so those signatures must keep that mangling to match
// res/libTTapp.so.
typedef long abi_long;           // NOLINT
typedef unsigned long abi_ulong; // NOLINT
```

- Verified: `abi_long` compiles to the exact same mangling as raw `long`
  (`_Z10f_abi_longl`), and `abi_ulong` as `_m`. The typedef indirection is only
  there to keep `clang-tidy`'s `google-runtime-int` quiet; the mangling is
  untouched.
- The `ssize_t` trap is documented in `src/nu2api/nucore/common.h:16-27`: raw
  `ssize_t` is `long` → mangled `_l` (wrong). Hence `isize` is redefined as
  `int32_t` on target builds (mangles `_i`) and `usize` = `size_t` (mangles
  `_j`). Both verified by compile (`f_ssize_t(long)` vs `f_size_t(unsigned int)`).
- `abi_long` is used in ABI-sensitive reconstructed signatures, for example
  `_make_words` in `src/legoapi/menus/core/text.cpp` and functions in
  `src/legoapi/audio/gamelib_ogg.cpp`—keep it where the target mangle requires it.
- Fixed-width types: `u8/i8/…/u64/i64` are defined in BOTH headers
  (`u8`, `i16`, `u32`, `f32`, `f64`, …). Duplicate typedefs of the same
  underlying type are legal in C++11, so both headers coexist in one TU
  (`fixed_width.h:7`).

**Which header when:**
- `common.h` (`src/nu2api/nucore/common.h`) — the engine header, widely included
  through `decomp.h` (`decomp.h` itself pulls in `common.h`).
  Defines the engine's `variptr_u` union
  (a real union of `void*`/`char*`/`i16*`/`u8*`/`u32*`/`usize` members, used by
  `VARIPTR` and the buffer globals declared in `src/globals.h`).
- `fixed_width.h` — used by the generated `*_types.h` scaffolding headers
  (nine current headers, including `legoapi_types.h`, `nu2api_*_types.h`, and
  `MechInputTouch_types.h`).
  It deliberately does not define `variptr_u`; scaffolding that needs the type,
  such as `src/gameapi/edtools/gameapi_edtools_types.h`, forward-declares it as
  a union. The complete definition still comes from `common.h`.

## 3. Calling conventions (i686, plain GCC cdecl — no fastcall/thiscall)

For ordinary C++ calls, including `this` and float arguments, parameters go
on the stack, right-to-left.
Do not add `regparm`, `fastcall`, `thiscall`, or similar calling-convention
attributes to reconstructed functions to improve a match score. Even when a
register appears to hold an argument in the original disassembly, reproduce
the behavior through ordinary C++ types and control flow, and document any
unresolved calling-convention difference instead of forcing an ABI in source.
One such unresolved local case is `pathEditor_DestroySharedNode`: the original
callee reads its pointer from `%eax`. This observation does not justify a
source-level calling-convention override.
Verified at `-O0` (frame pointers):

```
Widget::method(int a, int b):            # _ZN6Widget6methodEii
    push  %ebp
    lea   (%esp),%ebp
    mov   0x8(%ebp),%eax    # this  ← FIRST stack slot
    mov   (%eax),%edx       # m_data
    mov   0xc(%ebp),%eax    # a
    ...
    mov   0x10(%ebp),%eax   # b
    ret
```

At `-O3` the frame pointer vanishes: `this` at `0x4(%esp)`, args at
`0x8(%esp)`, `0xc(%esp)`. Float args are stack args too — `fmethod(float a,
double b)` reads `a` from `0xc(%ebp)` and `b` from `0x10(%ebp)`; **no xmm
registers are used for args** (SSE2 may be used *inside* the function body, as
that same function does with `cvtss2sd`/`addsd`).

Return values:
- `int`/pointers: `%eax`
- `long long`: `%edx:%eax` (verified `llmethod` → `mul` + `mov %ecx,%edx; ret`)
- `float`/`double`: x87 `st(0)` — verified `flds`/`fldl` immediately before
  `ret` (values are shuffled through x87 at -O0; the x87 is the *return*
  channel regardless)
- structs: **all sizes (1, 2, 4, 8, 12, 16 bytes) return through a hidden
  first `sret` pointer**, before `this` — GCC i386 defaults to
  `-fpcc-struct-return`. Verified: `r1..r12` all write through `0x8(%ebp)` and
  end `ret $4` (callee pops the sret pointer). The classic "1/2/4/8 bytes in
  %eax[:%edx]" rule does NOT apply to this toolchain; if you ever see a small
  struct returned in registers in the original, that function was built with
  `-freg-struct-return` (none known in this binary).

## 4. Struct layout / scalars (GCC i386 SysV, no -malign-double)

Runtime `sizeof` verification (`layout` binary run natively):

```
char 1  short 2  int 4  long 4  long long 8  float 4  double 8  ptr 4
bool 1   enum 4   enum class 4          # -fno-short-enums is the default
struct { char c; double d; }  = 12      # double aligned to 4, NOT 8!
struct { double d; char c; }  = 12
struct { char c; int i; }     = 8
struct { char c; double d; int i; } = 16
alignof({char;double}) = 4, but alignof(double) = 8
```

So max member alignment on i386 is 4 unless a member is 8+ bytes *and* you
build with `-malign-double` (the repo does not). A decompiled struct with a
`double` at a 4-aligned offset stays 4-aligned here — that's correct, don't
"fix" it to 8.

Original-layout checks must use `DECOMP_ASSERT` from `decomp.h`, not a
bare `static_assert`. It is a compile-time assertion in the original 32-bit
matching build and a no-op in host builds, whose pointer width legitimately
changes pointer-bearing structure sizes and offsets. Runtime code must use
typed members, typed pointer/array arithmetic, and `sizeof` for the ABI it is
actually compiled for. Localized byte arithmetic is only appropriate while a
range is genuinely unknown; original byte offsets and record sizes belong in
the decompilation/layout description and must not drive host allocation or
access.

Bitfields (verified with a runtime dump of
`{ unsigned a:3; b:5; c:12; d:7; }`, little-endian): fields pack LSB-first
within each 4-byte unit — `a` = bits 0-2, `b` = bits 3-7, `c` = bits 8-19,
`d` = bits 20-26 → bytes `ff ff ff 07`. Bit 0 of the first declared field is
the LSB of the first byte.

## 5. Ctor/dtor variants and vtables

### C1/C2/C3, D0/D1/D2

GCC 4.7 emits, for any class with a user dtor: `C1` (in-charge), `C2` (base),
and `D0` (deleting), `D1` (complete), `D2` (base). For plain classes `C1`/`C2`
and `D1`/`D2` collapse to one body with two symbol names (verified:
`_ZN6WidgetC1Ev` and `C2` at the same offset in `cc.o`; ditto `D1`/`D2`).
With virtual bases they become separate bodies (`vb.o`: `D::C2` at 0x9a,
`C1` at 0xc4) and `_ZTv0_n12_…` virtual thunks appear. C3 exists only in newer
ABI revisions — none here, and the repo's build keeps whatever GCC emits.

The original binary keeps *both* names, but as **two separate, duplicate
bodies**: `_ZN15NuSoundListenerC1Ev` @ 0x4e0 and `C2` @ 0x4a0 are 0x3d (61)
bytes of identical code, 0x40 apart (the identical run extends 64 bytes incl.
padding; re-verified byte-for-byte). Our rebuild
aliases them to one address, so at most one of the two split functions can
byte-match; both symbol names exist, so `scripts/checks/check_symbols.py` stays green.
Counts: original has 77 `C1Ev` + 77 `C2Ev` + 178 `D0Ev` + 224 `D1Ev` + 221
`D2Ev`; the current Bazel target provides the compiler-emitted variants (for
example `_ZN10NuFileBaseD0/D1/D2`).
There is **no C3, no `_ZTT` (construction vtable), no `_ZTc`** in the original
(0/0/0) → the original has no virtual-base classes; don't invent any.

Authoring rule: define the ctor/dtor out-of-line in the `.cpp` and all
variants come for free. The dtor will only produce `D0` if the dtor is
virtual (`D0` calls `operator delete`).

For virtual destruction followed by custom storage release, use a typed,
unqualified call such as `voice->~NuSoundVoice()`, then the original allocator's
free operation. This still dispatches to the complete-object destructor;
`delete voice` would also invoke storage deallocation. Do not reinterpret the
vtable entry as `void (*)(void *)`: WebAssembly complete-object destructors
return `this`, so that cast produces a runtime signature mismatch.

Verified with the repository toolchains on 2026-09-08: replacing the five raw
destructor calls in `NuSoundSystem` (`ReleaseVoice`, `ReleaseDecoder`,
`ReleaseEffect`, `ReleaseSample`, and `Shutdown`) with typed calls preserves
each Android function's objdiff score against the original, and the before/after
functions compare at 100%. This preserves existing matches; it does not mean
all five functions fully match the original. WebAssembly instead emits the
required pointer-returning indirect call and discards the result.

### Vtables

- Section `.data.rel.ro._ZTV<class>` (relocated read-only), symbol type `V`
  (weak) in both the current Bazel target and the original
  (`00669da0 V _ZTV13NuSoundEffect`) — this NDK GCC uses weak vtables, so the
  vtable appears in every TU that needs it and the linker keeps one copy.
  `check_symbols.py` only compares `T`/`W` text symbols, so vtables (`V`) are
  never counted as missing.
- Layout (verified in `virt.o` relocations and by reading the original
  binary's `_ZTV13NuSoundEffect`): `_ZTV` starts 8 bytes *before* the vptr —
  offset-to-top followed by the typeinfo pointer. For a simple primary vtable
  both are zero here; `-fno-rtti` makes the typeinfo pointer null rather than
  removing its slot. The object's vptr points at the **first virtual**. A virtual
  destructor occupies two adjacent slots (`D1` then `D0`).
- Virtual call codegen (verified, `-O3`):

```
vcaller(VBase*, int):            # p->f1(v)
    mov  0x20(%esp),%eax    # this
    mov  (%eax),%edx        # vptr
    mov  0x24(%esp),%ecx    # arg
    call *0x8(%edx)         # slot 2 = f1
```

- Emission rule: with at least one out-of-line virtual, the vtable is emitted
  weakly in that TU; with only inline virtuals it is emitted lazily on first
  use (verified: `unused.o` had none, `unused3.o` gained `V _ZTV10InlineOnly`
  once instantiated).
- **Slot order matters for byte-matching**: in the original
  `_ZTV13NuSoundEffect` (15 words = 13 virtuals) the dtor pair `D1`/`D0`
  sits at slots 6/7 (0-based from `_ZTV`, i.e. offsets 24/28; the 2-word zero
  prefix occupies slots 0/1) — the source header declares four virtuals
  before `~NuSoundEffect`: slots 2-5 = Initialise, Shutdown, Enable, Disable;
  slots 6/7 = `D1`/`D0`; slots 8-14 = AttachVoice, DetachVoice, ProcessVoice,
  AttachBus, DetachBus, ProcessBus, Process. When implementing a class, dump
  the original vtable (`nm res/libTTapp.so | grep _ZTV<class>` then read the
  word table) and order your header's virtuals to match. The vtable words are
  `R_386_RELATIVE` addends that resolve directly against `nm` addresses
  (verified: slot 2 = 0x31c790 `_ZN13NuSoundEffect10InitialiseEv`, etc.).

## 6. Thunks (_ZThn, _ZTv)

- `_ZThnNNN_…` — this-adjusting thunk for the second+ base of a class with
  multiple inheritance. Verified: `_ZThn4_N1D1gEv` is literally
  `subl $0x4, 0x4(%esp); jmp D::g` — the `NNN` is the byte adjustment applied
  to `this` before tail-jumping to the real method.
- `_ZTvNN…` — virtual-base (construction-vtable) thunks; `vb.o` emitted
  `_ZTv0_n12_…` for a virtual-base dtor. None in the original binary → no
  virtual bases in this codebase.
- The original has exactly 39 `_ZThn*`. Some belong to classes not yet
  implemented (including SceneObjectHelper, TTNetwork, NuSoundDecoderOGG, and
  NetworkObjectManager).
- Policy (`scripts/checks/check_symbols.py:91-94`): `_ZThn` thunks are filtered from
  the missing-symbol report — the compiler emits them automatically, so
  **never hand-write a thunk stub**; when you implement a real class with the
  matching multiple-inheritance layout the thunk appears by itself. If a
  stale stub object still provides one, remove the stub when the class lands.

## 7. extern "C"

Rule: a symbol with a plain (unmangled) name → `extern "C"`; anything mangled
(`_Z…`) → plain C++. Verified: `extern "C" int c_fn(int)` → `c_fn`,
C++ `cpp_fn` → `_Z6cpp_fni`.

The repo uses `extern "C"` pervasively for the C-flavored engine surface,
including selected globals in `src/globals.h`, the world headers under
`src/legoapi/world/`, `src/legoapi/characters/core/character.h`, gizmo and
editor-tool headers, `src/gameframework/saveload.h`, and
`src/gamelib/nuwind/nuwind.h`. One-off definitions include terrain functions
and `NuMain` in `src/batman.cpp`; JNI glue in `src/java/jni.h` uses it too.
When a plain C-looking name appears in the binary's symbol table, wrap it;
`i686-linux-android-nm res/libTTapp.so | grep ' T [A-Za-z_]'` lists candidates.

## 8. Static locals, guards, _GLOBAL__sub_I_

A function-local static with a non-trivial initializer compiles to a guard
+ `__cxa_guard_acquire`/`__cxa_guard_release` around the ctor, with the guard
variable named `_ZGVZ<func>E<var>` (verified: `_ZGVZ7get_foovE3foo`, local
`b`). Trivial statics (`static int counter = 42`) get no guard. With
`-fno-exceptions` the guard calls are **still emitted** — only
`__cxa_guard_abort` disappears (it was only reachable through the EH path).

- Original binary: 6 `ZGVZ*`, 1 `__cxa_guard_acquire` ref, and **325
  `_GLOBAL__sub_I_<file>.cpp`** symbols — the original had static
  initializers in nearly every TU.
- Current build snapshot: 35 `_GLOBAL__sub_I_*`.
- `_GLOBAL__sub_I_*` are compiler-generated and are **not** defined anywhere
  in `src/` — do not hand-write them; they appear automatically for any TU
  with file-scope init (e.g. an object with a ctor defined at file scope). The
  count will grow as matching work adds real classes; that is expected and
  `check_symbols.py` classifies these names as compiler-generated and excludes
  them from the must-provide set. Use them as TU evidence, not as symbols to
  hand-provision.

## 9. long long / i64 policy

The project convention avoids raw `long long`. The current tree has two
raw occurrences: the third-party `java/jni.h` typedef and one format-argument
cast in `ios_graphics.cpp`; normal engine code uses `i64`/`u64`. **Do not
"fix" `i64` signatures back to `long long`** to
match the original — `i64` mangles identically (`_x`/`_y`, §1) and the
original binary itself was compiled from `long long`-free-looking code where
it matters; where the binary really used `long long`, `i64` reproduces the
same symbol and the same codegen.

## 10. Type-choice recipes for agents

- **Plain 32-bit ints**: use `i32`/`u32`. Raw `int`/`unsigned int` are fine
  too (`_i`/`_j`) — but the codebase style is `i32`.
- **Use `abi_long`/`abi_ulong`** only where the binary's mangled symbol
  contains `_l`/`_m` (ogg/vorbis ABI stubs, legoapi). Same width as `i32`,
  different mangling — the symbol name is the arbiter.
- **`usize`/`isize`**: `usize` (=`size_t`, mangles `_j`) for sizes/counts,
  `isize` (=`int32_t` on target, mangles `_i`) for signed sizes. Never raw
  `ssize_t` (`_l` trap, `common.h:16-27`).
- **`char` is signed** by this GCC target's default: `char` → `_c`,
  `signed char` → `_a`, `unsigned char` → `_h`. If the binary demangles a
  parameter as `signed char` you must write `i8`/`signed char`, not `char`
  (they are distinct types and mangle differently).
- **`bool` vs `i32`**: `bool` is 1 byte and mangles `_b`. The engine's C side
  uses `i32` flags; the C++ side uses `bool`. Let the binary's mangling /
  Ghidra's size decide.
- **Enums are 4 bytes** with this build's defaults. A named enum parameter
  mangles as the enum type, not as a plain integer of the same width. A
  four-byte integer may be code-level compatible, but its mangled name differs;
  preserve the type shown by the target symbol.
- **Struct returns**: always via sret on this toolchain — author functions
  returning structs exactly as the binary does; don't try to make small
  structs return in registers.
- **Placeholder structs**: match Ghidra's layout exactly, byte for byte
  (`undefined` fields = `undefined1`…, see `decomp.h:14-30`; examples in
  `src/globals.h:13-148`). Naming: `field<NN>_0x<off>` per the placeholder
  convention. `undefined*` types are u8/u16/u32/u64 typedefs and are only
  layout, never codegen-affecting.
- **Promote recovered offsets into fields**: raw byte padding is temporary,
  not an implementation style. Once an access establishes a field's width and
  meaning, add it to the canonical structure and use the member everywhere.
  Prefer typed pointers, arrays, and function-pointer members over repeated
  `reinterpret_cast` arithmetic. This both resembles the original source and
  prevents 32-bit pointer offsets from leaking into host builds.
- **Serialized records are not necessarily runtime structs**: reconstruct the
  original sequence of typed reads and writes instead of copying file bytes
  into a runtime object. Packed fields may place a sample in low bits and
  tangents or flags above it; confirm the original mask and shift before
  assigning bitfields. A layout that consumes the correct total byte count can
  still be semantically wrong.

## 11. Open questions

- C1/C2 duplicate bodies in the original: some original TU was compiled with
  a compiler that emitted separate in-charge/base ctor copies (or with
  aliasing disabled). Only one of each pair can byte-match from our aliased
  build; the other stays unmatched — acceptable, but know that
  "C2 present but C1 not matching" is expected.
- `__cxa_guard_abort` is absent from our build (no exceptions); verify no
  original code path required it (guard already passed in the face of a
  concurrent ctor failure — not reachable without exceptions).
- The 13 unprovisioned `_ZThn` thunks will materialize when
  SceneObjectHelper / TTNetwork / NuSoundDecoderOGG / NetworkObjectManager
  are implemented with their real multiple-inheritance layouts — the exact
  adjustment constants (12/16/232/4) must match, so verify the vtable
  offsets of those classes first.
