# _saga_

![Progress](https://img.shields.io/badge/matching-44.60%25-orange)
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
| `MechInputTouch` | 28.3% | 34.6% |
| `editor` | 8.1% | 13.8% |
| `gameapi` | 31.4% | 18.0% |
| `gameframework` | 100.0% | 52.9% |
| `gamelib` | 26.1% | 24.0% |
| `java` | 96.1% | 73.1% |
| `legoapi` | 41.5% | 29.7% |
| `legoapi/actions` | 37.9% | 6.8% |
| `legoapi/ai` | 48.1% | 24.1% |
| `legoapi/audio` | 56.7% | 45.5% |
| `legoapi/characters` | 39.4% | 18.6% |
| `legoapi/core` | 31.0% | 21.1% |
| `legoapi/cutscenes` | 41.0% | 16.9% |
| `legoapi/gizmo` | 49.2% | 40.7% |
| `legoapi/gizmos` | 51.5% | 45.9% |
| `legoapi/items` | 41.6% | 38.4% |
| `legoapi/menus` | 31.1% | 27.5% |
| `legoapi/misc` | 31.2% | 13.0% |
| `legoapi/props` | 59.1% | 13.6% |
| `legoapi/render` | 40.5% | 27.0% |
| `legoapi/world` | 39.9% | 30.1% |
| `legogame` | 50.8% | 60.0% |
| `nu2api` | 69.8% | 56.9% |

<!-- matching-table-end -->

## Legal ⚖️

This educational research project contains reconstructed code only. Do not
commit game assets, leaked source code, or other copyrighted game data. The
original game remains the property of its owners.
