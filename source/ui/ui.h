#pragma once
#include <switch.h>
#include "../render/render.h"
#include "../player/player.h"

typedef enum {
    SCREEN_LIBRARY,
    SCREEN_ALBUM,
    SCREEN_NOW_PLAYING,
} Screen;

typedef struct {
    Screen  active;
    Player* player;
    int     library_selected;
    int     album_selected;
    int     mini_player_focused;
} UIState;

void ui_init(UIState* ui, Player* player);
void ui_update(UIState* ui, u64 held, u64 down);
void ui_draw(UIState* ui, RenderCtx* ctx);
void ui_draw_mini_player(UIState* ui, RenderCtx* ctx);