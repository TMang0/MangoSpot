#include "now_playing.h"
#include <stdio.h>

void now_playing_update(UIState* ui, u64 down) {
    // Play/pause con A
    if (down & HidNpadButton_A)
        player_toggle_pause(ui->player);

    // Siguiente con ZR o R
    if ((down & HidNpadButton_ZR) || (down & HidNpadButton_R))
        player_next(ui->player);

    // Anterior con ZL o L
    if ((down & HidNpadButton_ZL) || (down & HidNpadButton_L))
        player_prev(ui->player);

    // B → volver a library
    if (down & HidNpadButton_B)
        ui->active = SCREEN_LIBRARY;
}

void now_playing_draw(UIState* ui, RenderCtx* ctx) {
    Player* p = ui->player;
    Song*   s = p->current_song;
    if (!s) return;

    SDL_Color green   = {30,  215, 96,  255};
    SDL_Color white   = {255, 255, 255, 255};
    SDL_Color muted   = {179, 179, 179, 255};
    SDL_Color surface = {24,  24,  24,  255};
    SDL_Color bar_bg  = {60,  60,  60,  255};

    // Portada placeholder
    int cover_size = 260;
    int cover_x    = (SCREEN_W - cover_size) / 2;
    int cover_y    = 60;
    render_rect_rounded(ctx, cover_x, cover_y, cover_size, cover_size, 12, surface);
    char initial[2] = { s->album[0], '\0' };
    render_text_centered(ctx, initial,
        cover_x, cover_y + cover_size/2 - 30,
        cover_size, muted, ctx->font_bold);

    // Título y artista
    int info_y = cover_y + cover_size + 28;
    render_text_centered(ctx, s->title,  0, info_y,      SCREEN_W, white, ctx->font_bold);
    render_text_centered(ctx, s->artist, 0, info_y + 36, SCREEN_W, muted, ctx->font_regular);

    // Barra de progreso
    int bar_x = 160;
    int bar_y = info_y + 90;
    int bar_w = SCREEN_W - bar_x * 2;
    int bar_h = 4;
    render_bar(ctx, bar_x, bar_y, bar_w, bar_h, player_progress_pct(p), bar_bg, green);

    char time_cur[16], time_tot[16];
    render_format_time((int)p->progress_secs, time_cur, sizeof(time_cur));
    render_format_time(s->duration_secs,       time_tot, sizeof(time_tot));
    render_text(ctx, time_cur, bar_x,               bar_y + 14, muted, ctx->font_small);
    render_text(ctx, time_tot, bar_x + bar_w - 40,  bar_y + 14, muted, ctx->font_small);

    // Controles
    int ctrl_y = bar_y + 48;
    const char* pause_label = (p->state == PLAYER_PLAYING) ? "||" : ">";
    render_text_centered(ctx, "<< [L/ZL]", 60,            ctrl_y, 220,  white, ctx->font_regular);
    render_text_centered(ctx, pause_label,  SCREEN_W/2-30, ctrl_y, 60,   green, ctx->font_bold);
    render_text_centered(ctx, "[R/ZR] >>", SCREEN_W-280,  ctrl_y, 220,  white, ctx->font_regular);

    // Hint
    render_text_centered(ctx, "[B] Volver a biblioteca",
        0, SCREEN_H - 100, SCREEN_W, muted, ctx->font_small);
}