#include "album.h"
#include "utils/lang.h"
#include <stdio.h>

void album_update(UIState* ui, u64 down) {
    (void)ui;
    (void)down;
    // Ya no hay \u00e1lbumes locales; si el usuario llega aqu\u00ed, solo puede salir.
    if (down & HidNpadButton_B)
        ui->active = SCREEN_LIBRARY;
}

void album_draw(UIState* ui, RenderCtx* ctx) {
    (void)ui;

    SDL_Color muted = {179, 179, 179, 255};

    render_text_centered(ctx, "No hay \u00e1lbumes locales",
        0, 300, SCREEN_W, muted, ctx->font_regular);
    render_text_centered(ctx, lang_get(LK_BACK),
        0, 340, SCREEN_W, muted, ctx->font_small);
}