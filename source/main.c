#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <switch.h>
#include <stdio.h>
#include <string.h>
#include "applog.h"
#include "render/render.h"
#include "player/player.h"
#include "ui/ui.h"
#include "spotify/SpotifyClient.h"

int main(int argc, char* argv[]) {
    applog_init();  // stdout/stderr -> sdmc:/mangospot.log (survives crashes)
    printf("main: applog ready\n");

    romfsInit();  // mount romfs:/ (shared system fonts live here; mock songs removed)
    printf("main: romfs mounted\n");

    plInitialize(PlServiceType_User);  // Switch shared system fonts (render.c)
    printf("main: pl (shared fonts) initialized\n");

    socketInitializeDefault();
    printf("main: socket initialized\n");
    nifmInitialize(NifmServiceType_User);  // needed for Zeroconf mDNS to find our own IP
    printf("main: nifm initialized\n");

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_AUDIO);
    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096);
    printf("main: SDL + mixer initialized\n");
    applog_flush();

    if (spotify_client_start() != 0) {
        printf("main: Spotify Connect not started (see message above)\n");
    }
    printf("main: spotify_client_start returned\n");
    applog_flush();

    SDL_Window* window = SDL_CreateWindow(
        "Spotify Switch", 0, 0, 0, 0,
        SDL_WINDOW_FULLSCREEN
    );
    printf("main: window created\n");
    applog_flush();

    RenderCtx ctx;
    memset(&ctx, 0, sizeof(ctx));
    if (render_init(&ctx, window) != 0) {
        SDL_DestroyWindow(window);
        Mix_CloseAudio();
        SDL_Quit();
        return 1;
    }
    printf("main: render initialized, entering main loop\n");
    applog_flush();

    Player player;
    player_init(&player);

    UIState ui;
    ui_init(&ui, &player);

    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    PadState pad;
    padInitializeDefault(&pad);

    Uint32 last_tick = SDL_GetTicks();

    while (appletMainLoop()) {
        Uint32 now   = SDL_GetTicks();
        float  delta = (now - last_tick) / 1000.0f;
        last_tick    = now;

        padUpdate(&pad);
        u64 held = padGetButtons(&pad);
        u64 down = padGetButtonsDown(&pad);

        if (down & HidNpadButton_Plus) goto done;

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) goto done;
        }

        ui_update(&ui, held, down, delta);
        player_update(&player, delta);
        spotify_client_advance_playback(delta);

        render_clear(&ctx);
        ui_draw(&ui, &ctx);
        render_present(&ctx);
    }

done:
    render_destroy(&ctx);
    SDL_DestroyWindow(window);
    Mix_CloseAudio();
    SDL_Quit();
    nifmExit();
    socketExit();
    plExit();
    romfsExit();
    return 0;
}