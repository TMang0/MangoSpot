#pragma once
#include <switch.h>
#include <SDL2/SDL_mixer.h>    // ← agregar
#include "../data/mock_data.h"

typedef enum {
    PLAYER_STOPPED,
    PLAYER_PLAYING,
    PLAYER_PAUSED,
} PlayerState;

typedef struct {
    Song*        current_song;
    PlayerState  state;
    float        progress_secs;
    int          album_index;
    int          track_index;
    Mix_Music*   music;          // ← agregar
} Player;

void  player_init(Player* p);
void  player_update(Player* p, float delta);
void  player_play_track(Player* p, int album, int track);
void  player_toggle_pause(Player* p);
void  player_next(Player* p);
void  player_prev(Player* p);
float player_progress_pct(const Player* p);