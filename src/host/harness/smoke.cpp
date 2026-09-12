// Linux-only test executable. Link wrappers alter startup at the process
// boundary; the Android target and interactive host keep their normal loop.
#include <SDL3/SDL.h>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <unistd.h>
#include "globals.h"
#include "gameframework/saveload.h"
#include "host/harness/save.hpp"
#include "host/harness/window.hpp"
#include "legoapi/world/area.h"
#include "legoapi/characters/core/players.h"
#include "gameapi/gui/apimenu.h"

extern i32 LEVELCOUNT;
extern i32 GAMEDEMO;
extern i32 NewMode;
extern i32 Paused;
extern i32 LOADEROFF;
extern char g_language[16];
extern "C" void __real__Z7EndPermv();
extern "C" void __real__Z8LoadPermv();
extern "C" void __real_NuFrameBegin();

// Sanitizer errors remain fatal in the smoke test.
extern "C" const char *__asan_default_options() {
    return "halt_on_error=1";
}

extern "C" const char *__ubsan_default_options() {
    return "halt_on_error=1:print_stacktrace=1";
}

namespace {
    GAMESAVE_s fixture;
    const char *level_name = nullptr;
    const char *area_name = nullptr;
    LEVELDATA_s *destination = nullptr;
    std::atomic<Uint64> game_thread{0};
    std::atomic<Uint64> heartbeat{0};
    std::atomic<unsigned> healthy_frames{0};
    unsigned required_frames = 300;
    unsigned timeout_ms = 90000;
    unsigned stall_ms = 10000;
    i32 last_update = -1;
    f32 last_game_time = -1.0f;
    bool list_destinations = false;

    bool playable(const LEVELDATA_s &level) {
        return (level.flags & LEVEL_GAMEPLAY) != 0 &&
               (level.flags & (LEVEL_INTRO | LEVEL_MIDTRO | LEVEL_OUTRO | LEVEL_STATUS)) == 0;
    }

    void finish(int status, const char *reason) {
        // Do not wait for engine workers when one of them may be deadlocked.
        char message[512];
        const int length = snprintf(message, sizeof(message), "smoke: %s %s (healthy_frames=%u)\n",
                                    status == 0 ? "PASS" : "FAIL", reason, healthy_frames.load());
        // Avoid taking a stdio lock that a stalled engine thread could hold.
        if (length > 0) {
            const ssize_t ignored = write(STDERR_FILENO, message, static_cast<size_t>(length));
            (void)ignored;
        }
        _Exit(status);
    }

    int watchdog(void *) {
        const Uint64 start = SDL_GetTicks();
        unsigned previous = 0;
        Uint64 progress = start;
        while (true) {
            SDL_Delay(100);
            const Uint64 now = SDL_GetTicks();
            const unsigned frames = healthy_frames.load();
            if (frames != previous) {
                previous = frames;
                progress = now;
            }
            if (now - start >= timeout_ms)
                finish(124, "overall deadline exceeded");
            const Uint64 beat = heartbeat.load();
            if (beat != 0 && now - beat >= stall_ms)
                finish(124, "game thread stopped completing frames");
            if (frames != 0 && now - progress >= stall_ms)
                finish(124, "gameplay stopped advancing");
        }
    }

    unsigned positive(const char *value, bool allow_zero = false) {
        char *end;
        errno = 0;
        unsigned long n = strtoul(value, &end, 10);
        if (errno || *value == '-' || end == value || *end || (!allow_zero && n == 0) || n > 3600000)
            finish(2, allow_zero ? "numeric options must be in 0..3600000" : "numeric options must be in 1..3600000");
        return static_cast<unsigned>(n);
    }
} // namespace

extern "C" void __wrap__Z8LoadPermv() {
    // Use the engine's synchronous permanent-data loader to skip the legal,
    // language-selection and intro screens, without skipping asset setup.
    const i32 previous = LOADEROFF;
    LOADEROFF = 1;
    __real__Z8LoadPermv();
    LOADEROFF = previous;
}

extern "C" void __wrap__Z7EndPermv() {
    __real__Z7EndPermv();
    if (game_thread.load() != 0)
        return;
    if (list_destinations) {
        for (i32 i = 0; i < LEVELCOUNT; ++i) {
            if (!playable(LDataList[i]))
                continue;
            const i32 area = LDataList[i].area_index;
            printf("%s\t%s\n", area >= 0 && area < AREACOUNT ? ADataList[area].file : "-", LDataList[i].name);
        }
        fflush(stdout);
        _Exit(0);
    }
    if (area_name != nullptr) {
        for (i32 a = 0; a < AREACOUNT; ++a) {
            if (SDL_strcasecmp(ADataList[a].file, area_name) != 0)
                continue;
            for (i32 l = 0; l < ADataList[a].level_count; ++l) {
                const i32 index = ADataList[a].levels[l];
                if (index >= 0 && index < LEVELCOUNT && playable(LDataList[index])) {
                    destination = &LDataList[index];
                    break;
                }
            }
            break;
        }
    } else {
        for (i32 i = 0; i < LEVELCOUNT; ++i)
            if (SDL_strcasecmp(LDataList[i].name, level_name) == 0)
                destination = &LDataList[i];
    }
    if (destination == nullptr || !playable(*destination))
        finish(2, "unknown destination or destination is not a gameplay level");
    Game = fixture;
    BackupGame = fixture;
    memcard_autosaveenabled = 0;
    GAMEDEMO = 0;
    Level = destination->idx;
    NewMode = 0;
    PlayerProgress[0].active = 1;
    PlayerProgress[1].active = 0;
    MenuReset();
    fprintf(stderr, "smoke: loaded fixture; entering level=%s area=%d\n", destination->name, destination->area_index);
    heartbeat.store(SDL_GetTicks());
    game_thread.store(SDL_GetCurrentThreadID());
}

extern "C" void __wrap_NuFrameBegin() {
    __real_NuFrameBegin();
    if (SDL_GetCurrentThreadID() != game_thread.load())
        return;
    heartbeat.store(SDL_GetTicks());
    if (WORLD == nullptr || !WORLD->loaded || WORLD->current_level != destination || WORLD->sock_sys == nullptr ||
        Player[0] == nullptr || Paused != 0 || NewMode != 0 || NewLData != nullptr ||
        GameTimer.update_count == last_update)
        return;
    last_update = GameTimer.update_count;
    const NUVEC &p = Player[0]->apiobj.position;
    if (!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z) || !std::isfinite(GameTimer.time_elapsed))
        finish(1, "non-finite gameplay state");
    const u32 flags = Player[0]->apiobj.field_0x1f8;
    if ((flags & 0x1001) != 0x1001 || static_cast<i8>(flags) >= 0 || Player[0]->pad_gamepad == nullptr)
        return;
    if (GameTimer.time_elapsed <= last_game_time)
        return;
    last_game_time = GameTimer.time_elapsed;
    const unsigned frames = healthy_frames.fetch_add(1) + 1;
    if (frames == 1) {
        if (required_frames == 0)
            fprintf(stderr, "smoke: gameplay ready, running without a frame limit\n");
        else
            fprintf(stderr, "smoke: gameplay ready, checking %u frames\n", required_frames);
    }
    if (required_frames != 0 && frames >= required_frames)
        finish(0, "destination loaded and simulation advanced");
}

extern "C" int __wrap_main(int argc, char **argv) {
    const char *save = "res/SavedGames/SaveGame0.LEGO Star Wars - The Complete Saga_SavedGame";
    HostWindowOptions window;
    window.offscreen = true;
    window.mute = true;
    window.msaa = false;
    for (int i = 1; i < argc; ++i) {
        const char *arg = argv[i];
        if (strcmp(arg, "--help") == 0) {
            puts("Usage: saga_smoke (--level NAME | --area NAME) [--save FILE] [--frames 300]\n"
                 "                  [--timeout-ms 90000] [--stall-ms 10000] [--visible]\n"
                 "       saga_smoke --list (list area and gameplay-level names)\n"
                 "--frames 0 disables the frame limit; watchdog timeouts remain active.\n"
                 "Area selects its first gameplay level. Uses a hidden window and dummy audio by default.\n"
                 "Exit: 0 pass, 1 failure, 2 invalid input, 124 timeout; crashes retain their signal status.");
            fflush(stdout);
            _Exit(0);
        }
        if (strcmp(arg, "--list") == 0) {
            list_destinations = true;
            continue;
        }
        if (strcmp(arg, "--visible") == 0) {
            window.offscreen = false;
            continue;
        }
        if (++i >= argc)
            finish(2, "missing option value");
        if (strcmp(arg, "--level") == 0)
            level_name = argv[i];
        else if (strcmp(arg, "--area") == 0)
            area_name = argv[i];
        else if (strcmp(arg, "--save") == 0)
            save = argv[i];
        else if (strcmp(arg, "--frames") == 0)
            required_frames = positive(argv[i], true);
        else if (strcmp(arg, "--timeout-ms") == 0)
            timeout_ms = positive(argv[i]);
        else if (strcmp(arg, "--stall-ms") == 0)
            stall_ms = positive(argv[i]);
        else
            finish(2, "unknown option");
    }
    if (!list_destinations && (level_name == nullptr) == (area_name == nullptr))
        finish(2, "specify exactly one of --level or --area");
    if (list_destinations && (level_name != nullptr || area_name != nullptr))
        finish(2, "--list cannot be combined with a destination");
    if (!list_destinations && !host_read_game_fixture(save, fixture))
        finish(2, "fixture validation failed");
    char documents[256];
    if (!SDL_CreateDirectory(".work/smoke"))
        finish(1, "cannot create test documents directory");
    snprintf(documents, sizeof(documents), ".work/smoke/%ld-%llu/", static_cast<long>(getpid()),
             static_cast<unsigned long long>(SDL_GetTicksNS()));
    if (!SDL_CreateDirectory(documents))
        finish(1, "cannot create isolated test documents directory");
    window.documents_path = documents;
    strcpy(g_language, "en-us");
    SDL_Thread *thread = SDL_CreateThread(watchdog, "smoke-watchdog", nullptr);
    if (thread == nullptr)
        finish(1, "could not create watchdog");
    SDL_DetachThread(thread);
    host_run_window(window);
    finish(1, "window or engine exited before completing the test");
}
