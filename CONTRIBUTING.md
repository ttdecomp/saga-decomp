# Contributing 🤝

Use [README.md](README.md) for the shortest path from a new checkout to a
complete build. This document explains the platform setup and development
workflows in more detail.

Run every command below from the repository root. A value such as
`//src:saga_target` is a Bazel target label, not a filesystem path.

## Setup 🛠️

Install Bazelisk as `bazel`, or install the Bazel version named in
`.bazelversion`. Bazel downloads the Android and WebAssembly toolchains, the
project's Python runtime, and most source dependencies.

Run repository Python tools through `bazel run` or `bazel test`. Bazel then
uses the pinned Python 3.12.12 runtime. Running the files directly with
`python3` bypasses that environment.

Bazel downloads the pinned `clang-format` and `clang-tidy` binaries. They do
not need to be installed separately or added to `PATH`.

### Linux

Linux supports all three builds: `target`, `native`, and `wasm`.

Only the Linux `native` build uses system libraries. It intentionally produces
a 32-bit i686 executable, so the host needs a compiler capable of building
32-bit code and 32-bit development versions of SDL 3, Vorbis/Vorbisfile, EGL,
and GLES2. Bazel discovers those libraries through `pkg-config`. If they are
installed but not found, set `PKG_CONFIG_PATH` to their 32-bit `.pc` directory.

The Android `target` and browser `wasm` builds do not use those Linux system
libraries; Bazel downloads their toolchains and libraries.

### Windows

The `wasm` build works in the normal Windows Bazel environment. Build `target`
and `native` from an MSYS2 MINGW64 shell.

For both MSYS2 builds:

- set `BAZEL_SH` to MSYS2's `usr/bin/bash.exe`; and
- set `MSYS2_ARG_CONV_EXCL='*'` so MSYS2 does not rewrite Bazel arguments.

The Windows `native` build also needs the MINGW64 development packages for
GCC, pkg-config, SDL 3, Vorbis, GLES headers, and ANGLE. Set
`PKG_CONFIG_PATH=/mingw64/lib/pkgconfig`. Its commands must include
`--config=windows-mingw`.

These Windows native libraries are not used by the Android `target` build.

### macOS

The `wasm` build is supported on macOS.

The Intel limitation applies specifically to building the Android `target` on
macOS. Android NDK r8e is the old compiler package needed to reproduce the
original Android library. Its macOS tools were released for Intel x86-64, not
Apple Silicon. The target builds directly on an Intel Mac. On Apple Silicon,
Bazel must run in an x86-64 environment through Rosetta.

The repository contains native macOS rules, but the macOS `native` build is
not currently supported or tested in CI.

### Optional local game files

No game data or original binary is needed to compile the project. Two ignored
local files enable additional workflows:

| Local file                          | Purpose                                                       |
| ----------------------------------- | ------------------------------------------------------------- |
| `res/main.1060.com.wb.lego.tcs.obb` | Game assets used by the `native` and `wasm` builds at runtime |
| `res/libTTapp.so`                   | Original Android x86 library used only for binary matching    |

The project does not distribute either file. Never commit game assets,
original binaries, or leaked source code.

## Build 🔨

The repository builds the same reconstructed code for three different
purposes. The outputs are not interchangeable.

| Build    | Output                            | Purpose                                                              |
| -------- | --------------------------------- | -------------------------------------------------------------------- |
| `target` | Android x86 `libTTapp.so`         | Compare reconstructed machine code with the original Android library |
| `native` | Linux or Windows executable       | Run and debug reconstructed behavior on a desktop                    |
| `wasm`   | HTML, JavaScript, and WebAssembly | Run and debug reconstructed behavior in a browser                    |

Only `target` is used to measure binary matching. It is an Android shared
library, not a desktop executable. The `native` and `wasm` builds add host
support for testing behavior; their machine code is not compared with the
original Android library.

The host variants each have explicit debug and release configurations:

| Configuration    | Behavior                                                     |
| ---------------- | ------------------------------------------------------------ |
| `native`         | Debug native build; Linux enables AddressSanitizer and UBSan |
| `native_release` | Native build compiled with `-O2`, without sanitizers         |
| `wasm`           | Debug WebAssembly build                                      |
| `wasm_release`   | WebAssembly build compiled with `-O2`, without sanitizers    |

### Android target

```sh
bazel build --config=target //src:saga_target
```

The output is `bazel-bin/src/libTTapp.so`. Build it after changing shared game
or engine code, and before running any binary-matching tool.

Supported build hosts are Linux x86-64, Windows x86-64 through MSYS2, and
Intel macOS. Apple Silicon requires the Rosetta setup described above.

### Native executable

On Linux:

```sh
bazel build --config=native //src:saga_native
bazel build --config=native_release //src:saga_native
```

For direct gameplay regression checks on Linux, use the separate smoke build:

```sh
bazel run --config=native //src:run_smoke -- --area Negotiations --frames 300
bazel run --config=native //src:run_smoke -- --level Map --save '/path/to/fixture'
bazel run --config=native //src:run_smoke -- --list
```

It bypasses startup menus, validates and loads a save without modifying it, and
fails on crashes, sanitizer errors, stalled gameplay or an overall deadline.
It uses hidden graphics and dummy audio by default; an X display is required.
See [host utilities](doc/host-utilities.md#direct-gameplay-smoke-tests-linux) for
fixture selection, readiness checks, timeouts and exit statuses.

On Windows, from MSYS2 MINGW64:

```sh
bazel build --config=native --config=windows-mingw //src:saga_native
```

Build `native` when changing desktop runtime behavior. Linux produces
`bazel-bin/src/saga_native`; Windows produces the corresponding `.exe`.

To run the native build, first place the OBB from the setup section under
`res/`. Then open a window on Linux with:

```sh
bazel run --config=native //src:run_native -- window
```

On Windows, include its required configuration:

```sh
bazel run --config=native --config=windows-mingw //src:run_native -- window
```

Save files can be inspected and edited without an OBB or window:

```sh
bazel run --config=native //src:run_native -- save list
bazel run --config=native //src:run_native -- save schema --filter area_save
bazel run --config=native //src:run_native -- save edit '/path/to/save' coins=50000
bazel run --config=native //src:run_native -- save create '/path/to/new-save' --from '/path/to/template' coins=1000
```

Without a path, saves default to slot 0 under `res/SavedGames`. See
[host utilities](doc/host-utilities.md#save-file-inspection-and-editing) for the
property schema, parameter files, raw byte access, and fresh-creation defaults.

Window diagnostic modes:

```sh
# Rotate the camera automatically.
bazel run --config=native //src:run_native -- window --camera-orbit

# Move the camera with numpad 8/5/4/6; hold Shift to move faster.
bazel run --config=native //src:run_native -- window --camera-free
```

`//src:run_native` returns to the directory from which Bazel was invoked before
starting the program. Relative asset and output paths therefore resolve from
the repository root instead of Bazel's temporary run directory.

For an unattended Linux rendering check, this command runs deterministic
input without audio or a visible window and stores changed frames under
`.work/capture/`:

```sh
timeout 38s bazel run --config=native //src:run_native -- \
  window --offscreen --mute --script-input --capture
```

### WebAssembly

```sh
bazel build --config=wasm //src:saga_wasm
bazel build --config=wasm_release //src:saga_wasm
```

The outputs are `bazel-bin/src/saga.html`, `saga.js`, and `saga.wasm`. Build
this variant when changing browser support or portable host behavior.

With the OBB under `res/`, build and serve the browser version with:

```sh
bazel run --config=wasm //scripts:wasm_server
```

Open <http://127.0.0.1:8000/>. This repository server supplies the isolation
headers required by threaded WebAssembly and exposes the local OBB to the page.
To serve an OBB from another location, run:

```sh
bazel run --config=wasm //scripts:wasm_server -- \
  --obb /absolute/path/to/file.obb
```

## Development & scripts 🧩

### Repository layout

| Path                                   | Purpose                                                       |
| -------------------------------------- | ------------------------------------------------------------- |
| `src/`                                 | Reconstructed game, engine, platform, and desktop-host code   |
| `src/BUILD.bazel`                      | Defines build targets and assigns source files to them        |
| `bazel/android_per_file_copts.bazelrc` | Target-only optimization settings for individual source files |
| `scripts/checks/`                      | Automated repository checks                                   |
| `scripts/`                             | Bazel-run development and matching tools                      |
| `res/`                                 | Ignored local game data and original binary                   |
| `matching.json`                        | Committed matching results and original symbol information    |
| `doc/`                                 | Detailed agent and maintainer reference material              |

Host-only implementations belong under `src/host/`. This includes desktop
windows, input, graphics setup, diagnostics, and host file access. Do not add a
`HOST_BUILD` shortcut to shared game or engine code merely to make a desktop
test pass. Shared behavior should still reconstruct the original game.

### Normal development loop

1. Format changed C and C++ files with the repository configuration:

   ```sh
   bazel run //scripts/checks:clang_format -- \
     -i src/path/to/changed.cpp src/path/to/changed.h
   ```

2. Run the source-level checks:

   ```sh
   bazel test //scripts/checks:checks
   ```

3. Run clang-tidy for each build available on the current host:

   ```sh
   bazel build --config=target //src:clang_tidy_target
   bazel build --config=native //src:clang_tidy_native
   bazel build --config=native_release //src:clang_tidy_native
   bazel build --config=wasm //src:clang_tidy_wasm
   bazel build --config=wasm_release //src:clang_tidy_wasm
   ```

   macOS skips the unsupported `native` check. Apple Silicon also skips
   `target`, whose old Android toolchain requires an Intel environment.
   On Windows, add `--config=windows-mingw` to the native command.

4. Build `target` after shared game or engine changes. Also build `native`
   after desktop runtime changes and `wasm` after browser or portable-host
   changes. The commands and platform differences are in the Build section.

5. After building `target`, verify that it still defines every function
   expected by the original library:

   ```sh
   bazel run //scripts/checks:check_symbols -- \
     --original-symbols matching.json
   ```

The fast test target contains:

| Check                          | What it protects                                                             |
| ------------------------------ | ---------------------------------------------------------------------------- |
| `check_bazel_optimization_map` | Target optimization entries are valid and refer to the intended source files |
| `check_duplicate_definitions`  | C and C++ types do not have conflicting definitions                          |
| `check_host_boundary`          | Host-only behavior does not leak into shared game or engine code             |

The clang-tidy targets are separate because they are slower than the fast
suite. They are build targets rather than executable tests: each one attaches
clang-tidy actions to the C/C++ targets for its selected build configuration,
so Bazel supplies the correct defines, headers, and toolchain flags. The
targets make any reported violation fail the build. The symbol check is
separate because it needs the built target library. CI uses the original
symbol list stored in `matching.json`. If
`res/libTTapp.so` is available locally, omit `--original-symbols matching.json`
to read the expected symbols directly from that library. Add `--list` to print
individual missing symbols.

### Pre-commit hook

Enable the repository hook once per clone:

```sh
git config core.hooksPath .githooks
```

Every commit first runs `clang-format` over all C and C++ files under `src/`.
Formatting changes are staged automatically unless a file already contained
unstaged or untracked work; those files are left for review and the commit is
stopped. The hook then checks the staged diff for whitespace errors, runs the
three fast Bazel tests, and runs each clang-tidy target supported by the host.
If `res/libTTapp.so` is present, it also builds `target`, checks its symbols,
regenerates the full matching report, and stages the generated `matching.json`
and README progress table for the same commit.

Without `res/libTTapp.so`, the binary-dependent steps are skipped. The hook
does not generate `doc/pages/index.html`.

Run the same workflow without creating a commit with:

```sh
bazel run //scripts:pre_commit
```

### Binary matching

This workflow applies only to reproducing machine code in the Android `target`
build. It is not required for normal host debugging.

The supported original APK is:

| File                                          | SHA-256                                                            |
| --------------------------------------------- | ------------------------------------------------------------------ |
| `LEGO.SAGA.Android.ver.1.8.60.build.1060.apk` | `df69d191fdc5b4337dab7d1872cd9b7a6122df6d3b153e5df602aa3ff9fac7fb` |

Extract `lib/x86/libTTapp.so` from a legally obtained copy to
`res/libTTapp.so`. The extracted library's SHA-256 must be:

```text
d864055b1db5cc2ee2c16f7968ed68965b69f262ace6b6bfe43558296981c967
```

The matching scripts also require an external `objdiff-cli` executable on
`PATH`. Install the known-compatible revision with Rust's Cargo:

```sh
cargo install --git https://github.com/opensagadev/objdiff.git --branch codex/i386-linked-got objdiff-cli
```

> (note the `codex/i386-linked-got` branch; we are currently using this experimental branch for the decomp, but are undecided how to reconcile it with the changes we would like to contribute to upstream objdiff. In the future, we may return to installing the fork from main, or even the upstream `encounter/objdiff`)

To compare one function, build `target` and give its mangled symbol name to the
compact diff wrapper:

```sh
bazel build --config=target //src:saga_target
bazel run //scripts:objdiff_cli -- _Z5qrandv
```

The wrapper displays the instructions around each difference. In its
two-sided output, `-` and `~` identify original instructions; `+` and `>`
identify instructions from the current target build.

The objdiff GUI is completely optional. It only provides a visual interface
for browsing differences; building, matching reports, pre-commit, and CI do
not need it. Install the known-compatible revision with Rust's Cargo:

```sh
cargo install --git https://github.com/opensagadev/objdiff.git --branch codex/i386-linked-got objdiff-gui
# Generate its local project file and open the repository root:
bazel run //scripts:generate_objdiff_gui_config
# Then run the GUI:
objdiff -p .
```

Optimization settings are part of the expected machine code. Inspect the
actual target compile commands with:

```sh
bazel aquery --config=target \
  'mnemonic("CppCompile", deps(//src:saga_target))' --output=commands
```

`src/BUILD.bazel` controls which source file owns each function.
`bazel/android_per_file_copts.bazelrc` contains target-only optimization
exceptions. Moving a function between files can change its optimization,
symbol order, and nearby static data, so change either mapping only when the
original binary provides evidence for it.

### Matching report and Pages

After building `target`, and with `res/libTTapp.so` and `objdiff-cli`
available, generate the full report:

```sh
bazel run //scripts:generate_bazel_objdiff_report
```

This compares the complete original and rebuilt libraries. It writes
`matching.json`, recording each original function's address, size, match
percentage, and Bazel source unit. It also stores the original symbol list used
by CI and updates the matching badge and table in `README.md`.

Do not edit `matching.json` or the marked README section by hand. Commit both
generated files with the target changes that produced them.

Render a local copy of the GitHub Pages site with:

```sh
bazel run //scripts:plot_binary_match_map
```

This command only reads `matching.json` and writes the ignored
`doc/pages/index.html`; it does not build or compare either binary.

### Source conventions

- Never commit game assets, original binaries, or leaked source code.
- Preserve known symbol names, linkage, ABI, structure layout, and source order
  when matching depends on them.
- Use `PascalCase` for unknown functions and types.
- Use `snake_case` for members, parameters, and local variables.
- Use `UPPER_SNAKE_CASE` for enum values and macros.
- C-style structs and enums normally use `_s` and `_e` tags with uppercase
  typedef names. C++ classes use `PascalCase`.
- Prefer one canonical shared type definition over local placeholder copies.

Detailed ABI, GCC 4.7 code generation, source ownership, and diagnostic notes
are in the [agent and maintainer reference documentation](doc/main.md).

## CI 🤖

The repository has two GitHub Actions workflows. Both run for pull requests
targeting `main` and pushes to `main`. The Pages workflow also supports manual
dispatch.

### Bazel build workflow

`.github/workflows/build-bazel.yaml` runs:

- the three source-level checks;
- clang-tidy for the `target`, `native`, and `wasm` source modes on Linux;
- `target` on Linux, Windows, and Intel macOS;
- `wasm` on Linux, Windows, and macOS;
- the i686 `native` build on Linux;
- the MINGW64 `native` build on Windows; and
- the target symbol check on Linux.

CI cannot access `res/libTTapp.so`. Its symbol check therefore uses the
original symbol list committed inside `matching.json`. CI verifies the symbol
surface but does not calculate a new matching percentage.

Each job restores Bazel's download, repository, and build caches.
Pull-request runs can restore existing entries but do not save new ones. Pushes
to `main` may update the caches.

### Pages workflow

`.github/workflows/plot-pages.yaml` renders the site from the committed
`matching.json`, uploads `doc/pages/`, and deploys it to GitHub Pages. It does
not rebuild `target` or regenerate `matching.json`.

GitHub Actions invokes its checks and builds directly. It does not run the
local pre-commit hook.
