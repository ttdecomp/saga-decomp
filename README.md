# _saga_

![Progress](https://img.shields.io/badge/matching-39.58%25-orange)
[![Bazel build](https://github.com/ttdecomp/saga/actions/workflows/build-bazel.yaml/badge.svg)](https://github.com/ttdecomp/saga/actions/workflows/build-bazel.yaml)
[![Discord](https://img.shields.io/discord/1467775700894224555?color=%235865F2&logo=discord&logoColor=%23FFFFFF)](https://discord.gg/2HJuMtzA7q)
[![status & wasm build](https://img.shields.io/badge/status%20%26%20wasm%20build-click%20here-orange?style=flat)](https://opensaga.dev/)

This is a decompilation of _LEGO Star Wars: The Complete Saga_, based on the
Android x86 release. The repository builds three variants:

| Variant  | Purpose                                             |
| -------- | --------------------------------------------------- |
| `target` | Android x86 shared library used for binary matching |
| `native` | Linux or Windows diagnostic executable              |
| `wasm`   | Browser diagnostic build                            |

Game assets and the original binary are not included or required to compile.

## Build 🔨

You need Git and an x86-64 Linux C/C++ toolchain. The Linux `native`
variant uses system libraries: it needs your distribution's
32-bit SDL 3, Vorbis, EGL, GLES, and `pkg-config` development support. Bazel
downloads the toolchains and libraries used by `target` and `wasm`.

Install Bazelisk as `bazel` and it will select the version in `.bazelversion`.
Alternatively, install that Bazel version directly.

Clone and build every variant:

```sh
git clone https://github.com/ttdecomp/saga.git
cd saga

bazel build --config=target //src:saga_target
bazel build --config=native //src:saga_native
bazel build --config=wasm //src:saga_wasm
bazel test //scripts/checks:checks
```

Optimized, sanitizer-free host builds are available as
`--config=native_release` and `--config=wasm_release`; both compile with
`-O2`.

To run the native or browser build after supplying your own game assets, see
[CONTRIBUTING.md](CONTRIBUTING.md):

```sh
bazel run --config=native //src:run_native -- window
bazel run --config=wasm //scripts:wasm_server
```

<!-- matching-table-start -->

## Matching progress 📊

See https://ttdecomp.github.io/saga/

| Directory | Fuzzy % | Funcs % |
|---|---:|---:|
| `(root)` | 62.0% | 50.0% |
| `MechInputTouch` | 17.7% | 26.3% |
| `editor` | 3.5% | 5.7% |
| `gameapi` | 29.5% | 18.1% |
| `gameframework` | 100.0% | 52.9% |
| `gamelib` | 24.4% | 24.0% |
| `java` | 96.1% | 73.1% |
| `legoapi` | 37.2% | 29.2% |
| `legoapi/actions` | 32.7% | 6.8% |
| `legoapi/ai` | 48.0% | 24.1% |
| `legoapi/audio` | 54.5% | 45.5% |
| `legoapi/characters` | 33.2% | 18.7% |
| `legoapi/core` | 30.9% | 20.6% |
| `legoapi/cutscenes` | 36.6% | 15.7% |
| `legoapi/gizmo` | 43.4% | 37.0% |
| `legoapi/gizmos` | 50.4% | 47.3% |
| `legoapi/items` | 40.3% | 38.0% |
| `legoapi/menus` | 27.1% | 27.8% |
| `legoapi/misc` | 27.5% | 12.5% |
| `legoapi/props` | 52.2% | 13.9% |
| `legoapi/render` | 37.4% | 26.5% |
| `legoapi/world` | 30.9% | 30.2% |
| `legogame` | 50.8% | 60.0% |
| `nu2api` | 58.2% | 55.4% |

<!-- matching-table-end -->

## Legal ⚖️

This educational research project contains reconstructed code only. Do not
commit game assets, leaked source code, or other copyrighted game data. The
original game remains the property of its owners.
