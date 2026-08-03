#include "library.h"
#include "utils/lang.h"
#include <SDL2/SDL_image.h>
#include <stdio.h>

static SDL_Texture* g_gear_texture = NULL;

static SDL_Texture* load_gear_texture(RenderCtx* ctx) {
    if (g_gear_texture) return g_gear_texture;

    SDL_Surface* surf = IMG_Load("romfs:/images/gear.png");
    if (!surf) {
        printf("library: failed to load gear.png: %s\n", IMG_GetError());
        return NULL;
    }
    g_gear_texture = SDL_CreateTextureFromSurface(ctx->renderer, surf);
    SDL_FreeSurface(surf);
    return g_gear_texture;
}

void library_update(UIState* ui, u64 down) {
    // [X] abre el men\u00fa de ajustes.
    if (down & HidNpadButton_X) {
        ui->active = SCREEN_SETTINGS;
        ui->settings_selected = 0;
        return;
    }
    // La biblioteca local ya no existe; esta pantalla es ahora una vista de
    // bienvenida est\u00e1tica. No hay navegaci\u00f3n de lista ni apertura de \u00e1lbum.
}

void library_draw(UIState* ui, RenderCtx* ctx) {
    (void)ui;

    SDL_Color white   = {255, 255, 255, 255};
    SDL_Color muted   = {179, 179, 179, 255};
    SDL_Color green   = {30,  215, 96,  255};

    // Bot\u00f3n de tuerca en la esquina superior derecha.
    SDL_Texture* gear = load_gear_texture(ctx);
    int gear_w = 48;
    int gear_h = 40;
    int gear_x = SCREEN_W - 30 - gear_w;
    if (gear) {
        render_texture(ctx, gear, gear_x, 20, gear_w, gear_h);
    } else {
        ui_draw_gear_button(ctx, gear_x, 20, gear_h, white);
    }
    render_text(ctx, "[X]", SCREEN_W - 105, 58, muted, ctx->font_small);

    render_text_centered(ctx, lang_get(LK_WELCOME_TO), 0, 150, SCREEN_W, muted, ctx->font_regular);
    render_text_centered(ctx, "MangoSpot",    0, 200, SCREEN_W, green, ctx->font_bold);

    render_text_centered(ctx, lang_get(LK_CONNECT_FROM_SPOTIFY),
        0, 300, SCREEN_W, white, ctx->font_regular);
    render_text_centered(ctx, lang_get(LK_TO_START_PLAYING),
        0, 340, SCREEN_W, muted, ctx->font_small);
    render_text_centered(ctx, lang_get(LK_OPEN_DEVICES_MENU),
        0, 375, SCREEN_W, muted, ctx->font_small);

    render_text_centered(ctx, lang_get(LK_PRESS_DOWN_TO_FOCUS_MINI_PLAYER),
        0, 430, SCREEN_W, muted, ctx->font_small);
    render_text_centered(ctx, lang_get(LK_AND_USE_A_PAUSE_L_R_SKIP),
        0, 465, SCREEN_W, muted, ctx->font_small);

    render_text_centered(ctx, lang_get(LK_OPEN_PLAYER),
        0, 530, SCREEN_W, muted, ctx->font_small);
    render_text_centered(ctx, lang_get(LK_EXIT),
        0, 565, SCREEN_W, muted, ctx->font_small);
}