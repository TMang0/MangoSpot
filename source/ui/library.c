#include "library.h"
#include <stdio.h>

void library_update(UIState* ui, u64 down) {
    (void)ui;
    (void)down;
    // La biblioteca local ya no existe; esta pantalla es ahora una vista de
    // bienvenida estática. No hay navegación de lista ni apertura de álbum.
}

void library_draw(UIState* ui, RenderCtx* ctx) {
    (void)ui;

    SDL_Color white   = {255, 255, 255, 255};
    SDL_Color muted   = {179, 179, 179, 255};
    SDL_Color green   = {30,  215, 96,  255};

    render_text_centered(ctx, "Bienvenido a", 0, 170, SCREEN_W, muted, ctx->font_regular);
    render_text_centered(ctx, "MangoSpot",    0, 220, SCREEN_W, green, ctx->font_bold);

    render_text_centered(ctx, "Conéctate desde Spotify",
        0, 330, SCREEN_W, white, ctx->font_regular);
    render_text_centered(ctx, "para empezar a reproducir",
        0, 370, SCREEN_W, muted, ctx->font_small);

    render_text_centered(ctx, "[Y] Abrir reproductor    [+] Salir",
        0, 520, SCREEN_W, muted, ctx->font_small);
}