#include "host/harness/audio.hpp"
#include "host/harness/load.hpp"
#include "host/harness/save.hpp"
#include "host/harness/window.hpp"
#include "host/platform/runtime.hpp"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <unistd.h>

extern char g_language[16];

namespace {

    enum class HostUtility {
        audio,
        load,
        window,
    };

    struct HostHarnessOptions {
        HostUtility utility = HostUtility::window;
        HostLoadOptions load;
        HostWindowOptions window;
    };

    void host_print_usage(const char *program) {
        printf("Usage: %s <utility> [options]\n", program);
        printf("\nHost utilities:\n");
        printf("  audio                  Verify audio playback through the host device\n");
        printf("  load [list|extract]    Inspect or extract game data\n");
        printf("  save [list|schema|edit|create] Inspect, explain, edit, or create a save file\n");
        printf("  window [options]       Run the game in an SDL window\n");
        printf("\nRun '%s <utility> --help' for utility-specific options.\n", program);
    }

    void host_print_audio_usage(const char *program) {
        printf("Usage: %s audio\n", program);
    }

    void host_print_load_usage(const char *program) {
        printf("Usage:\n");
        printf("  %s load\n", program);
        printf("  %s load list [filter]\n", program);
        printf("  %s load extract <dat-path> [output]\n", program);
    }

    void host_print_window_usage(const char *program) {
        printf("Usage: %s window [options]\n", program);
        printf("\nOptions:\n");
        printf("  --capture              Capture changed frames under .work/capture\n");
        printf("  --trace-movement       Log player movement state twice per second\n");
        printf("  --script-input         Exercise the new-game menu flow\n");
        printf("  --script-load          Exercise the load-game menu flow\n");
        printf("  --script-play          Exercise the new-game flow and player movement\n");
        printf("  --script-action        Exercise the new-game flow and one player saber action\n");
        printf("  --script-pause         Exercise scripted play, then open and resume the pause menu\n");
        printf("  --camera-orbit         Rotate camera yaw 360 degrees over 10 seconds in the Cantina\n");
        printf("  --camera-free          Free camera: numpad 8/5/4/6 rotate; hold Shift to move\n");
        printf("  --offscreen            Create a hidden, non-focusable window\n");
        printf("  --mute                 Use SDL's dummy audio driver\n");
        printf("  --fps                  Show a top-left FPS counter\n");
        printf("  --no-msaa              Disable native-host 4x multisampling\n");
        printf("  --no-portals           Disable portal culling for host comparison captures\n");
        printf("  --script-tail-ms <ms>  Wait after scripted input completes (default: 8000)\n");
        printf("  --timeout-ms <ms>      Stop the window utility after this time (default: unlimited)\n");
    }

    bool host_parse_milliseconds(const char *option, const char *value, u64 &result) {
        errno = 0;
        char *end = nullptr;
        const u64 parsed = strtoull(value, &end, 10);
        if (*value == '-' || errno != 0 || end == value || *end != '\0') {
            fprintf(stderr, "Invalid value for %s: %s\n", option, value);
            return false;
        }
        result = parsed;
        return true;
    }

    bool host_parse_audio_arguments(i32 argc, char **argv, const char *program) {
        if (argc == 0) {
            return true;
        }
        fprintf(stderr, "Unexpected argument for audio: %s\n", argv[0]);
        host_print_audio_usage(program);
        return false;
    }

    bool host_parse_load_arguments(i32 argc, char **argv, const char *program, HostLoadOptions &options) {
        if (argc == 0) {
            return true;
        }
        if (strcmp(argv[0], "list") == 0) {
            if (argc > 2) {
                fprintf(stderr, "Too many arguments for load list\n");
                host_print_load_usage(program);
                return false;
            }
            options.action = HostLoadAction::list;
            options.filter = argc == 2 ? argv[1] : nullptr;
            return true;
        }
        if (strcmp(argv[0], "extract") == 0) {
            if (argc < 2 || argc > 3) {
                fprintf(stderr, "load extract requires a DAT path and accepts one optional output path\n");
                host_print_load_usage(program);
                return false;
            }
            options.action = HostLoadAction::extract;
            options.dat_path = argv[1];
            options.output_path = argc == 3 ? argv[2] : ".work/extracted.bin";
            return true;
        }
        fprintf(stderr, "Unknown load action: %s\n", argv[0]);
        host_print_load_usage(program);
        return false;
    }

    bool host_parse_window_arguments(i32 argc, char **argv, const char *program, HostWindowOptions &options) {
        for (i32 i = 0; i < argc; ++i) {
            const char *argument = argv[i];
            if (strcmp(argument, "--capture") == 0) {
                options.capture = true;
            } else if (strcmp(argument, "--trace-movement") == 0) {
                options.trace_movement = true;
            } else if (strcmp(argument, "--script-input") == 0) {
                options.script_input = true;
            } else if (strcmp(argument, "--script-load") == 0) {
                options.script_input = true;
                options.script_load = true;
            } else if (strcmp(argument, "--script-play") == 0) {
                options.script_input = true;
                options.script_play = true;
            } else if (strcmp(argument, "--script-action") == 0) {
                options.script_input = true;
                options.script_play = true;
                options.script_action = true;
            } else if (strcmp(argument, "--script-pause") == 0) {
                options.script_input = true;
                options.script_play = true;
                options.script_pause = true;
            } else if (strcmp(argument, "--camera-orbit") == 0) {
                options.script_input = true;
                options.script_play = true;
                options.camera_orbit = true;
            } else if (strcmp(argument, "--camera-free") == 0) {
                options.script_input = true;
                options.script_play = true;
                options.camera_free = true;
            } else if (strcmp(argument, "--offscreen") == 0) {
                options.offscreen = true;
            } else if (strcmp(argument, "--mute") == 0) {
                options.mute = true;
            } else if (strcmp(argument, "--fps") == 0) {
                options.show_fps = true;
            } else if (strcmp(argument, "--no-msaa") == 0) {
                options.msaa = false;
            } else if (strcmp(argument, "--no-portals") == 0) {
                options.portals = false;
            } else if (strcmp(argument, "--script-tail-ms") == 0 || strcmp(argument, "--timeout-ms") == 0) {
                if (++i == argc) {
                    fprintf(stderr, "Missing value for %s\n", argument);
                    host_print_window_usage(program);
                    return false;
                }
                u64 &destination =
                    strcmp(argument, "--script-tail-ms") == 0 ? options.script_tail_ms : options.timeout_ms;
                if (!host_parse_milliseconds(argument, argv[i], destination)) {
                    host_print_window_usage(program);
                    return false;
                }
            } else {
                fprintf(stderr, "Unknown window option: %s\n", argument);
                host_print_window_usage(program);
                return false;
            }
        }
        if (options.camera_orbit && options.camera_free) {
            fprintf(stderr, "--camera-orbit and --camera-free cannot be used together\n");
            return false;
        }
        return true;
    }

    enum class HostParseResult {
        run,
        help,
        error,
    };

    HostParseResult host_parse_arguments(i32 argc, char **argv, HostHarnessOptions &options) {
        const char *program = argv[0];
        if (argc < 2) {
            host_print_usage(program);
            return HostParseResult::error;
        }
        if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
            host_print_usage(program);
            return HostParseResult::help;
        }

        const i32 utility_argc = argc - 2;
        char **utility_argv = argv + 2;
        if (strcmp(argv[1], "audio") == 0) {
            options.utility = HostUtility::audio;
            if (utility_argc == 1 && strcmp(utility_argv[0], "--help") == 0) {
                host_print_audio_usage(program);
                return HostParseResult::help;
            }
            return host_parse_audio_arguments(utility_argc, utility_argv, program) ? HostParseResult::run
                                                                                   : HostParseResult::error;
        }
        if (strcmp(argv[1], "load") == 0) {
            options.utility = HostUtility::load;
            if (utility_argc == 1 && strcmp(utility_argv[0], "--help") == 0) {
                host_print_load_usage(program);
                return HostParseResult::help;
            }
            return host_parse_load_arguments(utility_argc, utility_argv, program, options.load)
                       ? HostParseResult::run
                       : HostParseResult::error;
        }
        if (strcmp(argv[1], "window") == 0) {
            options.utility = HostUtility::window;
            if (utility_argc == 1 && strcmp(utility_argv[0], "--help") == 0) {
                host_print_window_usage(program);
                return HostParseResult::help;
            }
            return host_parse_window_arguments(utility_argc, utility_argv, program, options.window)
                       ? HostParseResult::run
                       : HostParseResult::error;
        }

        fprintf(stderr, "Unknown host utility: %s\n", argv[1]);
        host_print_usage(program);
        return HostParseResult::error;
    }

    void host_initialize_language() {
        // On device Java fills the engine locale string via nativeSetLanguage
        // before NuMain runs; emulate that once for every host utility.
        const char *lang = getenv("LANG");
        if (lang == nullptr) {
            lang = "en-us";
        }
        snprintf(g_language, sizeof(g_language), "%.15s", lang);
        for (char *character = g_language; *character != '\0'; ++character) {
            if (*character == '_') {
                *character = '-';
            }
        }
    }

    void host_finish_engine_session(i32 status) {
        // NuMain is a process-lifetime entry point and the window/audio
        // utilities stop observing it before its worker threads have exited.
        // Keep the hard process boundary local to those utilities until their
        // reconstructed shutdown path can join every engine thread.
        fflush(nullptr);
        _exit(status);
    }

} // namespace

i32 main(i32 argc, char **argv) {
    HostPlatformPrepareArguments(&argc, &argv);

    if (argc >= 2 && strcmp(argv[1], "save") == 0) {
        return host_run_save(argc - 2, argv + 2);
    }

    HostHarnessOptions options;
    const HostParseResult result = host_parse_arguments(argc, argv, options);
    if (result != HostParseResult::run) {
        return result == HostParseResult::help ? 0 : 1;
    }

    host_initialize_language();
    i32 utility_result = 1;
    switch (options.utility) {
        case HostUtility::audio:
            utility_result = host_run_audio();
            break;
        case HostUtility::load:
            utility_result = host_run_load(options.load);
            break;
        case HostUtility::window:
            utility_result = host_run_window(options.window);
            break;
    }

    if (options.utility == HostUtility::load) {
        return utility_result;
    }
    host_finish_engine_session(utility_result);
}
