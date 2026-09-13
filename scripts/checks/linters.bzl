"""Bazel-native clang-tidy checks for the project's C and C++ targets."""

load("@bazel_tools//tools/build_defs/cc:action_names.bzl", "ACTION_NAMES")
load("@bazel_tools//tools/cpp:toolchain_utils.bzl", "find_cpp_toolchain")
load("@rules_cc//cc:defs.bzl", "CcInfo", "cc_common")

_SOURCE_EXTENSIONS = ["c", "C", "cc", "cpp", "cxx", "c++"]
_DISABLED_FEATURES = ["layering_check"]

# These original .c-named translation units contain C++ constructors and are
# compiled as C++ by the matching and host builds. Lint them in that language
# as well; clang-tidy cannot infer the Bazel per-file -x override from a suffix.
_CXX_NAMED_C_SOURCES = [
    "src/nu2api/nu3d/android/nuptl_android.c",
    "src/nu2api/nu3d/android/nudlist_android.c",
    "src/nu2api/nu3d/android/nuprim_android.c",
    "src/nu2api/nu3d/glutils.c",
    "src/nu2api/nu3d/android/nurain_android.c",
    "src/nu2api/nucore/android/nuthread.c",
    "src/nu2api/nucore/android/nutime_android.c",
]

def _prefixed(values, prefix):
    result = []
    for value in values:
        result.extend([prefix, value])
    return result

def _safe_flag(flag):
    """Remove GCC/MSVC flags that clang's parser cannot consume."""
    if flag in [
        "-fno-canonical-system-headers",
        "-fstack-usage",
        "/nologo",
        "/COMPILER_MSVC",
        "/showIncludes",
        "/experimental:external",
    ]:
        return []
    if flag.startswith("-W") or flag.startswith("/W") or flag.startswith("/wd") or flag.startswith("/external"):
        return []
    if flag.startswith("/std:"):
        return ["-std=" + flag.removeprefix("/std:")]
    if flag.startswith("/D"):
        return ["-" + flag[1:]]
    if flag.startswith("/FI"):
        return ["-include", flag.removeprefix("/FI")]
    if flag.startswith("/I"):
        return ["-iquote", flag.removeprefix("/I")]
    if flag in ["/MD", "/MDd", "/MT", "/MTd"]:
        return ["-D_MT"]
    if flag.startswith("/"):
        return []
    return [flag]

def _safe_flags(flags):
    result = []
    for flag in flags:
        result.extend(_safe_flag(flag))
    return result

def _compiler_args(ctx, compilation_context, source):
    c_source = source.extension == "c" and source.short_path not in _CXX_NAMED_C_SOURCES
    action_name = ACTION_NAMES.c_compile if c_source else ACTION_NAMES.cpp_compile
    user_flags = ctx.fragments.cpp.copts
    if action_name == ACTION_NAMES.cpp_compile:
        user_flags = ctx.fragments.cpp.cxxopts + user_flags

    cc_toolchain = find_cpp_toolchain(ctx)
    feature_configuration = cc_common.configure_features(
        ctx = ctx,
        cc_toolchain = cc_toolchain,
        requested_features = ctx.features,
        unsupported_features = ctx.disabled_features + _DISABLED_FEATURES,
    )
    variables = cc_common.create_compile_variables(
        feature_configuration = feature_configuration,
        cc_toolchain = cc_toolchain,
        user_compile_flags = user_flags,
    )
    toolchain_flags = cc_common.get_memory_inefficient_command_line(
        feature_configuration = feature_configuration,
        action_name = action_name,
        variables = variables,
    )
    rule_flags = list(getattr(ctx.rule.attr, "copts", []))
    if action_name == ACTION_NAMES.cpp_compile:
        rule_flags += list(getattr(ctx.rule.attr, "cxxopts", []))

    result = _safe_flags(toolchain_flags + rule_flags)
    if cc_toolchain.compiler == "mingw-gcc":
        # The Windows LLVM distribution defaults to MSVC. Parse with the same
        # ABI and system headers as the selected MinGW compiler instead.
        compiler = cc_common.get_tool_for_action(
            feature_configuration = feature_configuration,
            action_name = action_name,
        ).replace("\\", "/")
        result += ["--target=x86_64-w64-windows-gnu", "--sysroot=" + compiler.rsplit("/", 2)[0]]
    result.append("-xc" if action_name == ACTION_NAMES.c_compile else "-xc++")

    for define in compilation_context.defines.to_list():
        result.append("-D" + define)
    for define in compilation_context.local_defines.to_list():
        result.append("-D" + define)

    result += _prefixed(compilation_context.framework_includes.to_list(), "-F")
    result += _prefixed(compilation_context.includes.to_list(), "-I")
    result += _prefixed(compilation_context.quote_includes.to_list(), "-iquote")
    result += _prefixed(compilation_context.system_includes.to_list(), "-isystem")
    result += _prefixed(compilation_context.external_includes.to_list(), "-isystem")
    return result, feature_configuration, variables, action_name

def _clang_tidy_aspect(extra_args = []):
    def _impl(target, ctx):
        if CcInfo not in target or not hasattr(ctx.rule.attr, "srcs"):
            return []

        sources = [
            source
            for source in ctx.rule.files.srcs
            if source.is_source and source.extension in _SOURCE_EXTENSIONS
        ]
        if not sources:
            return []

        compilation_context = target[CcInfo].compilation_context
        inputs = depset(
            direct = [ctx.file._config],
            transitive = [compilation_context.headers, ctx.attr._clang_headers[DefaultInfo].files],
        )
        cc_toolchain = find_cpp_toolchain(ctx)
        outputs = []

        for source in sources:
            output = ctx.actions.declare_file(
                "{}_clang_tidy/{}.stamp".format(target.label.name, source.short_path),
            )
            compiler_args, feature_configuration, variables, action_name = _compiler_args(
                ctx,
                compilation_context,
                source,
            )
            env = dict(cc_common.get_environment_variables(
                feature_configuration = feature_configuration,
                action_name = action_name,
                variables = variables,
            ))
            env.update({
                "MSYS_ARG_CONV_EXCL": "*",
                "MSYS_NO_PATHCONV": "1",
            })

            arguments = ctx.actions.args()
            arguments.add("--config-file=" + ctx.file._config.path)
            arguments.add_all(ctx.attr._extra_args)

            # native_binary relocates clang-tidy away from its LLVM installation.
            # Supply its builtin headers explicitly, including in sandboxed actions.
            for header in ctx.attr._clang_headers[DefaultInfo].files.to_list():
                if header.path.endswith("/include/stddef.h") and "/lib/clang/" in header.path:
                    arguments.add("--extra-arg=-resource-dir=" + header.dirname.removesuffix("/include"))
                    break
            arguments.add(source.path)
            arguments.add("--")
            arguments.add_all(compiler_args)
            arguments.use_param_file("@%s", use_always = True)
            arguments.set_param_file_format("multiline")

            runner_arguments = ctx.actions.args()
            runner_arguments.add(output.path)
            runner_arguments.add(ctx.executable._clang_tidy.path)

            ctx.actions.run(
                executable = ctx.executable._runner,
                arguments = [runner_arguments, arguments],
                inputs = depset(direct = [source], transitive = [inputs]),
                outputs = [output],
                tools = [ctx.executable._clang_tidy, cc_toolchain.all_files],
                env = env,
                mnemonic = "ClangTidy",
                progress_message = "Linting %{label}:" + source.basename,
            )
            outputs.append(output)

        # Validation outputs execute whenever a rule depending on these targets
        # is built, without printing hundreds of report paths on success.
        return [OutputGroupInfo(_validation = depset(outputs))]

    return aspect(
        implementation = _impl,
        attrs = {
            "_cc_toolchain": attr.label(
                default = "@bazel_tools//tools/cpp:current_cc_toolchain",
            ),
            "_clang_tidy": attr.label(
                default = "//scripts/checks:clang_tidy",
                cfg = "exec",
                executable = True,
            ),
            "_clang_headers": attr.label(
                default = "@llvm_tools_llvm//:include",
                cfg = "exec",
            ),
            "_config": attr.label(
                default = "//:.clang-tidy",
                allow_single_file = True,
            ),
            "_extra_args": attr.string_list(default = extra_args),
            "_runner": attr.label(
                default = "//scripts/checks:run_clang_tidy",
                cfg = "exec",
                executable = True,
            ),
        },
        fragments = ["cpp"],
        required_providers = [CcInfo],
        toolchains = ["@bazel_tools//tools/cpp:toolchain_type"],
    )

clang_tidy = _clang_tidy_aspect()

# The legacy Android GCC toolchain exposes its sysroot but not its architecture
# in CcInfo. Tell clang's parser that this build uses 32-bit Android x86.
clang_tidy_android = _clang_tidy_aspect(
    extra_args = ["--extra-arg=--target=i686-linux-android"],
)

clang_tidy_wasm = _clang_tidy_aspect(
    extra_args = ["--extra-arg=--target=wasm32-unknown-emscripten"],
)

def _clang_tidy_check_impl(_ctx):
    return DefaultInfo()

def _clang_tidy_check(aspect):
    return rule(
        implementation = _clang_tidy_check_impl,
        attrs = {
            "srcs": attr.label_list(aspects = [aspect]),
        },
    )

def _wasm_transition_impl(_settings, _attr):
    return {
        "//command_line_option:compiler": "emscripten",
        "//command_line_option:cpu": "wasm",
        "//command_line_option:custom_malloc": "@emsdk//emscripten_toolchain:malloc",
        "//command_line_option:dynamic_mode": "off",
        "//command_line_option:platforms": ["@emsdk//:platform_wasm"],
    }

_wasm_transition = transition(
    implementation = _wasm_transition_impl,
    inputs = [],
    outputs = [
        "//command_line_option:compiler",
        "//command_line_option:cpu",
        "//command_line_option:custom_malloc",
        "//command_line_option:dynamic_mode",
        "//command_line_option:platforms",
    ],
)

# saga_wasm_objects receives the Emscripten toolchain through wasm_cc_binary in
# the normal build. Apply the equivalent transition before attaching the aspect.
clang_tidy_wasm_check = rule(
    implementation = _clang_tidy_check_impl,
    attrs = {
        "srcs": attr.label_list(
            aspects = [clang_tidy_wasm],
            cfg = _wasm_transition,
        ),
        "_allowlist_function_transition": attr.label(
            default = "@bazel_tools//tools/allowlists/function_transition_allowlist",
        ),
    },
)

clang_tidy_check = _clang_tidy_check(clang_tidy)
clang_tidy_android_check = _clang_tidy_check(clang_tidy_android)
