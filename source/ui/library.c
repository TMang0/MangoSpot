#include "library.h"
#include <stdio.h>

void library_update(UIState* ui, u64 down) {
    if (down & HidNpadButton_Down)
        ui->library_selected = (ui->library_selected + 1) % library_count;

    if (down & HidNpadButton_Up)
        ui->library_selected = (ui->library_selected - 1 + library_count) % library_count;

    // A → abrir álbum (no reproducir directo)
    if (down & HidNpadButton_A) {
        ui->album_selected = 0;
        ui->active = SCREEN_ALBUM;
    }
}

void library_draw(UIState* ui, RenderCtx* ctx) {
    SDL_Color white   = {255, 255, 255, 255};
    SDL_Color muted   = {179, 179, 179, 255};
    SDL_Color green   = {30,  215, 96,  255};
    SDL_Color surface = {24,  24,  24,  255};
    SDL_Color sel     = {40,  40,  40,  255};

    render_text(ctx, "Tu biblioteca", 60, 40, white, ctx->font_bold);

    int item_h  = 80;
    int start_y = 110;

    for (int i = 0; i < library_count; i++) {
        Album* a      = &library[i];
        int    item_y = start_y + i * (item_h + 8);
        int    is_sel = (i == ui->library_selected);

        SDL_Color bg = is_sel ? sel : surface;
        render_rect_rounded(ctx, 60, item_y, SCREEN_W - 120, item_h, 8, bg);

        if (i == ui->player->album_index)
            render_rect(ctx, 60, item_y, 4, item_h, green);

        render_rect_rounded(ctx, 72, item_y + 10, 60, 60, 6, (SDL_Color){50,50,50,255});
        char initial[2] = { a->name[0], '\0' };
        render_text_centered(ctx, initial, 72, item_y + 26, 60, muted, ctx->font_bold);

        render_text(ctx, a->name,   148, item_y + 16, is_sel ? white : muted, ctx->font_bold);
        render_text(ctx, a->artist, 148, item_y + 46, muted, ctx->font_small);

        char count[32];
        snprintf(count, sizeof(count), "%d canciones", a->track_count);
        render_text(ctx, count, SCREEN_W - 280, item_y + 30, muted, ctx->font_small);
    }
}