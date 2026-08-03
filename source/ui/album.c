#include "album.h"
#include <stdio.h>

void album_update(UIState* ui, u64 down) {
    (void)ui;
    (void)down;
    // Ya no hay álbumes locales; si el usuario llega aquí, solo puede salir.
    if (down & HidNpadButton_B)
        ui->active = SCREEN_LIBRARY;
}

void album_draw(UIState* ui, RenderCtx* ctx) {
    (void)ui;

    SDL_Color muted = {179, 179, 179, 255};

    render_text_centered(ctx, "No hay álbumes locales",
        0, 300, SCREEN_W, muted, ctx->font_regular);
    render_text_centered(ctx, "[B] Volver",
        0, 340, SCREEN_W, muted, ctx->font_small);
}