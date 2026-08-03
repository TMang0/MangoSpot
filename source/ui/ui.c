#include "ui.h"
#include "library.h"
#include "album.h"
#include "now_playing.h"
#include "../spotify/SpotifyClient.h"

void ui_init(UIState* ui, Player* player) {
    ui->active              = SCREEN_LIBRARY;
    ui->player              = player;
    ui->library_selected    = 0;
    ui->album_selected      = 0;
    ui->mini_player_focused = 0;
}

void ui_update(UIState* ui, u64 held, u64 down) {
    (void)held;

    // Solo la pantalla grande del reproductor tiene su propia navegación.
    if (ui->active == SCREEN_NOW_PLAYING) {
        now_playing_update(ui, down);
        return;
    }

    // Bajar al mini player con Abajo desde la pantalla de bienvenida.
    if (!ui->mini_player_focused && (down & HidNpadButton_Down)) {
        ui->mini_player_focused = 1;
        return;
    }

    // Subir desde mini player
    if (ui->mini_player_focused) {
        if (down & HidNpadButton_Up) {
            ui->mini_player_focused = 0;
            return;
        }
        SpotifyNowPlaying np;
        spotify_client_get_now_playing(&np);
        // Controles del mini player - si estamos conectados a Spotify de
        // verdad, mandan comandos reales en lugar de tocar el mock.
        if (down & HidNpadButton_A) {
            if (np.is_connected) spotify_client_toggle_pause();
            else player_toggle_pause(ui->player);
        }
        if ((down & HidNpadButton_R) || (down & HidNpadButton_ZR)) {
            if (np.is_connected) spotify_client_next();
            else player_next(ui->player);
        }
        if ((down & HidNpadButton_L) || (down & HidNpadButton_ZL)) {
            if (np.is_connected) spotify_client_prev();
            else player_prev(ui->player);
        }
        // Y → abrir now playing
        if (down & HidNpadButton_Y)
            ui->active = SCREEN_NOW_PLAYING;
        return;
    }

    // La pantalla de bienvenida no tiene navegación propia; el álbum
    // tampoco tiene contenido, solo [B] para volver.
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
    SpotifyNowPlaying np;
    spotify_client_get_now_playing(&np);

    const char* title;
    const char* artist;
    char album_initial;
    int is_playing;
    float progress_pct;

    if (np.is_connected) {
        title  = np.title;
        artist = np.artist;
        album_initial = np.album[0] ? np.album[0] : '?';
        is_playing = np.is_playing;
        int total_secs = (int)(np.duration_ms / 1000);
        progress_pct = total_secs > 0 ? (np.position_secs / (float)total_secs) : 0.0f;
    } else {
        Player* p = ui->player;
        if (p->current_song) {
            title  = p->current_song->title;
            artist = p->current_song->artist;
            album_initial = p->current_song->album[0];
            is_playing = (p->state == PLAYER_PLAYING);
            progress_pct = player_progress_pct(p);
        } else {
            title  = "No conectado";
            artist = "Abre Spotify y elige MangoSpot";
            album_initial = '?';
            is_playing = 0;
            progress_pct = 0.0f;
        }
    }

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

    // Portada: igual que en la pantalla grande, usa la real si ya está
    // disponible, si no cae al placeholder con la inicial del álbum.
    SDL_Texture* cover = np.is_connected ? render_get_spotify_cover_art(ctx) : NULL;
    if (cover) {
        render_texture(ctx, cover, margin, y + 12, 56, 56);
    } else {
        render_rect_rounded(ctx, margin, y + 12, 56, 56, 6, (SDL_Color){40,40,40,255});
        char initial[2] = { album_initial, '\0' };
        render_text_centered(ctx, initial, margin, y + 28, 56, muted, ctx->font_small);
    }

    // Título y artista
    render_text(ctx, title,  margin + 68, y + 14, white, ctx->font_regular);
    render_text(ctx, artist, margin + 68, y + 40, muted, ctx->font_small);

    // Controles
    int ctrl_x = SCREEN_W - 220;
    render_text(ctx, "[L]", ctrl_x,       y + 28, muted,  ctx->font_small);
    render_text(ctx,
        is_playing ? "||" : ">",
        ctrl_x + 60,  y + 24, green,  ctx->font_bold);
    render_text(ctx, "[R]", ctrl_x + 120, y + 28, muted,  ctx->font_small);

    // Hint cuando está enfocado
    if (ui->mini_player_focused)
        render_text_centered(ctx, "[Y] Abrir | [A] Play/Pause | [L/R] Skip",
            0, y - 24, SCREEN_W, muted, ctx->font_small);

    // Barra de progreso
    render_bar(ctx, 0, y - 3, SCREEN_W, 3, progress_pct, bar_bg, green);
}