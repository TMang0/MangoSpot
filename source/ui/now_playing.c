#include "now_playing.h"
#include <stdio.h>
#include "utils/lang.h"
#include "../spotify/SpotifyClient.h"

void now_playing_update(UIState* ui, u64 down) {
    SpotifyNowPlaying np;
    spotify_client_get_now_playing(&np);

    if (np.is_connected) {
        // Conectado a Spotify real: los controles mandan comandos SPIRC de
        // verdad (el reproductor mock no suena, así que tocarlo aquí no
        // haría nada audible).
        if (down & HidNpadButton_A)
            spotify_client_toggle_pause();
        if ((down & HidNpadButton_ZR) || (down & HidNpadButton_R))
            spotify_client_next();
        if ((down & HidNpadButton_ZL) || (down & HidNpadButton_L))
            spotify_client_prev();
        if (down & HidNpadButton_X)
            spotify_client_toggle_like();
    } else {
        // Play/pause con A
        if (down & HidNpadButton_A)
            player_toggle_pause(ui->player);

        // Siguiente con ZR o R
        if ((down & HidNpadButton_ZR) || (down & HidNpadButton_R))
            player_next(ui->player);

        // Anterior con ZL o L
        if ((down & HidNpadButton_ZL) || (down & HidNpadButton_L))
            player_prev(ui->player);
    }

    // B → volver a library
    if (down & HidNpadButton_B)
        ui->active = SCREEN_LIBRARY;
}

void now_playing_draw(UIState* ui, RenderCtx* ctx) {
    SpotifyNowPlaying np;
    spotify_client_get_now_playing(&np);

    const char* title;
    const char* artist;
    char album_initial;
    int is_playing;
    int cur_secs, total_secs;
    float progress_pct;

    if (np.is_connected) {
        // Real Spotify Connect track info takes priority over the local
        // mock-library demo data once we're actually connected.
        title  = np.title;
        artist = np.artist;
        album_initial = np.album[0] ? np.album[0] : '?';
        is_playing = np.is_playing;
        total_secs = (int)(np.duration_ms / 1000);
        cur_secs   = (int)np.position_secs;
        progress_pct = total_secs > 0 ? (np.position_secs / (float)total_secs) : 0.0f;

        // Estado optimista: mientras el comando viaja a Spotify, el título
        // queda vacío; mostramos un indicador de "cambiando" en lugar de
        // dejar la canción anterior congelada en pantalla.
        if (title[0] == '\0') {
            title  = lang_get(LK_CHANGING_SONG);
            artist = "";
            progress_pct = 0.0f;
            cur_secs = 0;
        }
    } else {
        Player* p = ui->player;
        Song*   s = p->current_song;
        if (!s) {
            // Sin canción local y sin conexión a Spotify: pantalla de
            // bienvenida en lugar de quedarse en negro.
            SDL_Color muted = {179, 179, 179, 255};
            SDL_Color green = {30,  215, 96,  255};
            render_text_centered(ctx, "MangoSpot", 0, 260, SCREEN_W, green, ctx->font_bold);
            render_text_centered(ctx, lang_get(LK_NO_ACTIVE_PLAYBACK),
                0, 340, SCREEN_W, muted, ctx->font_regular);
            render_text_centered(ctx, lang_get(LK_CONNECT_FROM_SPOTIFY_AND_SELECT_DEVICE),
                0, 380, SCREEN_W, muted, ctx->font_small);
            render_text_centered(ctx, lang_get(LK_BACK),
                0, 520, SCREEN_W, muted, ctx->font_small);
            return;
        }
        title  = s->title;
        artist = s->artist;
        album_initial = s->album[0];
        is_playing = (p->state == PLAYER_PLAYING);
        total_secs = s->duration_secs;
        cur_secs   = (int)p->progress_secs;
        progress_pct = player_progress_pct(p);
    }

    SDL_Color green   = {30,  215, 96,  255};
    SDL_Color white   = {255, 255, 255, 255};
    SDL_Color muted   = {179, 179, 179, 255};
    SDL_Color surface = {24,  24,  24,  255};
    SDL_Color bar_bg  = {60,  60,  60,  255};

    // Portada: si ya se descargó y decodificó la portada real (solo aplica
    // a la sesión real de Spotify Connect, el reproductor mock no tiene
    // imágenes), se dibuja esa; si no, se cae al placeholder con la
    // inicial del álbum.
    int cover_size = 260;
    int cover_x    = (SCREEN_W - cover_size) / 2;
    int cover_y    = 60;
    SDL_Texture* cover = np.is_connected ? render_get_spotify_cover_art(ctx) : NULL;
    if (cover) {
        render_texture(ctx, cover, cover_x, cover_y, cover_size, cover_size);
    } else {
        render_rect_rounded(ctx, cover_x, cover_y, cover_size, cover_size, 12, surface);
        char initial[2] = { album_initial, '\0' };
        render_text_centered(ctx, initial,
            cover_x, cover_y + cover_size/2 - 30,
            cover_size, muted, ctx->font_bold);
    }

    // Título y artista
    int info_y = cover_y + cover_size + 28;
    render_text_centered(ctx, title,  0, info_y,      SCREEN_W, white, ctx->font_bold);
    render_text_centered(ctx, artist, 0, info_y + 36, SCREEN_W, muted, ctx->font_regular);

    // Barra de progreso
    int bar_x = 160;
    int bar_y = info_y + 90;
    int bar_w = SCREEN_W - bar_x * 2;
    int bar_h = 4;
    render_bar(ctx, bar_x, bar_y, bar_w, bar_h, progress_pct, bar_bg, green);

    char time_cur[16], time_tot[16];
    render_format_time(cur_secs,   time_cur, sizeof(time_cur));
    render_format_time(total_secs, time_tot, sizeof(time_tot));
    render_text(ctx, time_cur, bar_x,               bar_y + 14, muted, ctx->font_small);
    render_text(ctx, time_tot, bar_x + bar_w - 40,  bar_y + 14, muted, ctx->font_small);

    // Controles
    int ctrl_y = bar_y + 48;
    char pause_label[16];
    snprintf(pause_label, sizeof(pause_label), "(A) %s",
        lang_get(is_playing ? LK_PAUSE_ICON : LK_PLAY_ICON));
    render_text_centered(ctx, lang_get(LK_PREV_TRACK), 60,            ctrl_y, 220,  white, ctx->font_regular);
    render_text_centered(ctx, pause_label,  SCREEN_W/2-30, ctrl_y, 100,  green, ctx->font_bold);
    render_text_centered(ctx, lang_get(LK_NEXT_TRACK), SCREEN_W-280,  ctrl_y, 220,  white, ctx->font_regular);

    // Favoritos
    int fav_y = ctrl_y + 44;
    const char* fav_label = np.is_connected
        ? lang_get(np.is_liked ? LK_REMOVE_FROM_FAVORITES : LK_ADD_TO_FAVORITES)
        : "";
    if (fav_label[0]) {
        render_text_centered(ctx, fav_label, 0, fav_y, SCREEN_W,
            np.is_liked ? (SDL_Color){255, 90, 90, 255} : muted, ctx->font_small);
    }

    // Hint
    render_text_centered(ctx, lang_get(LK_BACK_TO_LIBRARY),
        0, SCREEN_H - 100, SCREEN_W, muted, ctx->font_small);
}