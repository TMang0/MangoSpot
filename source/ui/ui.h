#pragma once
#include <switch.h>
#include "../render/render.h"
#include "../player/player.h"

typedef enum {
    SCREEN_LIBRARY,
    SCREEN_ALBUM,
    SCREEN_NOW_PLAYING,
    SCREEN_SETTINGS,
    SCREEN_CREDITS,
} Screen;

typedef struct {
    Screen  active;
    Player* player;
    int     library_selected;
    int     album_selected;
    int     mini_player_focused;
    int     settings_selected;
    float   settings_highlight_y;
} UIState;

void ui_init(UIState* ui, Player* player);
void ui_update(UIState* ui, u64 held, u64 down, float delta);
void ui_draw(UIState* ui, RenderCtx* ctx);
void ui_draw_mini_player(UIState* ui, RenderCtx* ctx);
void ui_draw_gear_button(RenderCtx* ctx, int x, int y, int size, SDL_Color color);