#include "player.h"
#include <stdlib.h>

void player_init(Player* p) {
    p->album_index   = 0;
    p->track_index   = 0;
    p->current_song  = NULL;
    p->state         = PLAYER_PAUSED;
    p->progress_secs = 0.0f;
    p->music         = NULL;
}

static void player_load_and_play(Player* p) {
    if (p->music) {
        Mix_HaltMusic();
        Mix_FreeMusic(p->music);
        p->music = NULL;
    }

    p->progress_secs = 0.0f;
    p->state         = PLAYER_PLAYING;

    if (p->current_song && p->current_song->file_path) {
        p->music = Mix_LoadMUS(p->current_song->file_path);
        if (p->music) Mix_PlayMusic(p->music, 1);
    }
}

void player_update(Player* p, float delta) {
    if (p->state != PLAYER_PLAYING || !p->current_song) return;

    p->progress_secs += delta;

    if (p->progress_secs >= p->current_song->duration_secs)
        player_next(p);
}

void player_play_track(Player* p, int album, int track) {
    if (library_count == 0 || album < 0 || album >= library_count) return;
    if (track < 0 || track >= library[album].track_count) return;

    p->album_index  = album;
    p->track_index  = track;
    p->current_song = &library[album].tracks[track];
    player_load_and_play(p);
}

void player_toggle_pause(Player* p) {
    if (!p->current_song) return;

    if (p->state == PLAYER_PLAYING) {
        p->state = PLAYER_PAUSED;
        Mix_PauseMusic();
    } else {
        // Si no hay música cargada, cargarla ahora
        if (!p->music && p->current_song->file_path) {
            p->music = Mix_LoadMUS(p->current_song->file_path);
        }
        p->state = PLAYER_PLAYING;
        if (p->music) {
            if (Mix_PausedMusic())
                Mix_ResumeMusic();
            else
                Mix_PlayMusic(p->music, 1);
        }
    }
}

void player_next(Player* p) {
    if (library_count == 0 || !p->current_song) return;

    p->track_index++;
    if (p->track_index >= library[p->album_index].track_count) {
        p->album_index = (p->album_index + 1) % library_count;
        p->track_index = 0;
    }
    p->current_song = &library[p->album_index].tracks[p->track_index];
    player_load_and_play(p);
}

void player_prev(Player* p) {
    if (library_count == 0 || !p->current_song) return;

    if (p->progress_secs > 3.0f) {
        p->progress_secs = 0.0f;
        if (p->music) {
            Mix_HaltMusic();
            Mix_PlayMusic(p->music, 1);
        }
        return;
    }
    p->track_index--;
    if (p->track_index < 0) {
        p->album_index = (p->album_index - 1 + library_count) % library_count;
        p->track_index = library[p->album_index].track_count - 1;
    }
    p->current_song = &library[p->album_index].tracks[p->track_index];
    player_load_and_play(p);
}

float player_progress_pct(const Player* p) {
    if (!p->current_song || p->current_song->duration_secs == 0) return 0.0f;
    return p->progress_secs / (float)p->current_song->duration_secs;
}