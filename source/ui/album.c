#include "album.h"
#include <stdio.h>

void album_update(UIState* ui, u64 down) {
    Album* a = &library[ui->library_selected];

    if (down & HidNpadButton_Down)
        ui->album_selected = (ui->album_selected + 1) % a->track_count;

    if (down & HidNpadButton_Up)
        ui->album_selected = (ui->album_selected - 1 + a->track_count) % a->track_count;

    // A → reproducir canción seleccionada
    if (down & HidNpadButton_A) {
        player_play_track(ui->player, ui->library_selected, ui->album_selected);
        ui->active = SCREEN_NOW_PLAYING;
    }

    // B → volver a library
    if (down & HidNpadButton_B)
        ui->active = SCREEN_LIBRARY;
}

void album_draw(UIState* ui, RenderCtx* ctx) {
    Album* a = &library[ui->library_selected];

    SDL_Color white   = {255, 255, 255, 255};
    SDL_Color muted   = {179, 179, 179, 255};
    SDL_Color green   = {30,  215, 96,  255};
    SDL_Color surface = {24,  24,  24,  255};
    SDL_Color sel     = {40,  40,  40,  255};

    // Header del álbum
    render_rect_rounded(ctx, 60, 30, 100, 100, 8, surface);
    char initial[2] = { a->name[0], '\0' };
    render_text_centered(ctx, initial, 60, 68, 100, muted, ctx->font_bold);

    render_text(ctx, a->name,   180, 40,  white, ctx->font_bold);
    render_text(ctx, a->artist, 180, 72,  muted, ctx->font_regular);

    char year[16];
    snprintf(year, sizeof(year), "%d", a->year);
    render_text(ctx, year, 180, 98, muted, ctx->font_small);

    // Lista de tracks
    int item_h  = 60;
    int start_y = 150;

    for (int i = 0; i < a->track_count; i++) {
        Song* s      = &a->tracks[i];
        int   item_y = start_y + i * (item_h + 4);
        int   is_sel = (i == ui->album_selected);
        int   is_playing = (ui->player->album_index == ui->library_selected &&
                            ui->player->track_index == i);

        SDL_Color bg = is_sel ? sel : (SDL_Color){0,0,0,0};
        if (is_sel) render_rect_rounded(ctx, 60, item_y, SCREEN_W - 120, item_h, 6, bg);

        // Número o indicador de reproducción
        if (is_playing) {
            render_text(ctx, "▶", 72, item_y + 18, green, ctx->font_small);
        } else {
            char num[4];
            snprintf(num, sizeof(num), "%d", i + 1);
            render_text(ctx, num, 72, item_y + 18, muted, ctx->font_small);
        }

        // Título
        render_text(ctx, s->title, 110, item_y + 10, is_sel ? white : muted, ctx->font_regular);

        // Duración
        char dur[16];
        int m = s->duration_secs / 60;
        int sec = s->duration_secs % 60;
        snprintf(dur, sizeof(dur), "%d:%02d", m, sec);
        render_text(ctx, dur, SCREEN_W - 160, item_y + 10, muted, ctx->font_small);
    }

    // Hint de navegación
    render_text_centered(ctx, "[B] Volver   [A] Reproducir",
        0, SCREEN_H - 100, SCREEN_W, muted, ctx->font_small);
}