#include "ui.h"
#include "library.h"
#include "album.h"
#include "now_playing.h"

void ui_init(UIState* ui, Player* player) {
    ui->active              = SCREEN_LIBRARY;
    ui->player              = player;
    ui->library_selected    = 0;
    ui->album_selected      = 0;
    ui->mini_player_focused = 0;
}

void ui_update(UIState* ui, u64 held, u64 down) {
    // Solo library y album tienen mini player interactivo
    if (ui->active == SCREEN_NOW_PLAYING) {
        now_playing_update(ui, down);
        return;
    }

    // Bajar al mini player
    if (!ui->mini_player_focused && (down & HidNpadButton_Down)) {
        // Si ya estamos en el último item, bajar al mini player
        int at_bottom = 0;
        if (ui->active == SCREEN_LIBRARY && ui->library_selected == library_count - 1)
            at_bottom = 1;
        if (ui->active == SCREEN_ALBUM) {
            Album* a = &library[ui->library_selected];
            if (ui->album_selected == a->track_count - 1)
                at_bottom = 1;
        }
        if (at_bottom) {
            ui->mini_player_focused = 1;
            return;
        }
    }

    // Subir desde mini player
    if (ui->mini_player_focused) {
        if (down & HidNpadButton_Up) {
            ui->mini_player_focused = 0;
            return;
        }
        // Controles del mini player
        if (down & HidNpadButton_A)
            player_toggle_pause(ui->player);
        if ((down & HidNpadButton_R) || (down & HidNpadButton_ZR))
            player_next(ui->player);
        if ((down & HidNpadButton_L) || (down & HidNpadButton_ZL))
            player_prev(ui->player);
        // Y → abrir now playing
        if (down & HidNpadButton_Y)
            ui->active = SCREEN_NOW_PLAYING;
        return;
    }

    // Navegación normal
    switch (ui->active) {
        case SCREEN_LIBRARY:
            library_update(ui, down);
            break;
        case SCREEN_ALBUM:
            album_update(ui, down);
            break;
        default:
            break;
    }
}

void ui_draw(UIState* ui, RenderCtx* ctx) {
    switch (ui->active) {
        case SCREEN_LIBRARY:
            library_draw(ui, ctx);
            break;
        case SCREEN_ALBUM:
            album_draw(ui, ctx);
            break;
        case SCREEN_NOW_PLAYING:
            now_playing_draw(ui, ctx);
            break;
    }

    if (ui->active != SCREEN_NOW_PLAYING)
        ui_draw_mini_player(ui, ctx);
}

void ui_draw_mini_player(UIState* ui, RenderCtx* ctx) {
    Player* p = ui->player;
    if (!p->current_song) return;

    SDL_Color bg      = {18,  18,  18,  255};
    SDL_Color white   = {255, 255, 255, 255};
    SDL_Color muted   = {179, 179, 179, 255};
    SDL_Color green   = {30,  215, 96,  255};
    SDL_Color bar_bg  = {60,  60,  60,  255};
    // Borde verde si está enfocado
    SDL_Color border  = ui->mini_player_focused
                        ? (SDL_Color){30, 215, 96, 255}
                        : (SDL_Color){50, 50,  50, 255};

    int h      = 80;
    int y      = SCREEN_H - h;
    int margin = 16;

    render_rect(ctx, 0, y, SCREEN_W, h, bg);
    render_rect(ctx, 0, y, SCREEN_W, 2, border);  // borde superior

    // Portada placeholder
    render_rect_rounded(ctx, margin, y + 12, 56, 56, 6, (SDL_Color){40,40,40,255});
    char initial[2] = { p->current_song->album[0], '\0' };
    render_text_centered(ctx, initial, margin, y + 28, 56, muted, ctx->font_small);

    // Título y artista
    render_text(ctx, p->current_song->title,  margin + 68, y + 14, white, ctx->font_regular);
    render_text(ctx, p->current_song->artist, margin + 68, y + 40, muted, ctx->font_small);

    // Controles
    int ctrl_x = SCREEN_W - 220;
    render_text(ctx, "[L]", ctrl_x,       y + 28, muted,  ctx->font_small);
    render_text(ctx,
        (p->state == PLAYER_PLAYING) ? "||" : ">",
        ctrl_x + 60,  y + 24, green,  ctx->font_bold);
    render_text(ctx, "[R]", ctrl_x + 120, y + 28, muted,  ctx->font_small);

    // Hint cuando está enfocado
    if (ui->mini_player_focused)
        render_text_centered(ctx, "[Y] Abrir | [A] Play/Pause | [L/R] Skip",
            0, y - 24, SCREEN_W, muted, ctx->font_small);

    // Barra de progreso
    render_bar(ctx, 0, y - 3, SCREEN_W, 3, player_progress_pct(p), bar_bg, green);
}