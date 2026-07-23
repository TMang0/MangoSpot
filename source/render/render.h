#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

// Colores del tema (estilo Spotify)
#define COLOR_BG          { 12,  12,  12, 255}   // negro casi puro
#define COLOR_SURFACE     { 24,  24,  24, 255}   // gris muy oscuro (cards)
#define COLOR_SURFACE2    { 40,  40,  40, 255}   // gris medio (hover)
#define COLOR_GREEN       { 30, 215, 96,  255}   // verde Spotify
#define COLOR_TEXT        {255, 255, 255, 255}   // blanco
#define COLOR_TEXT_MUTED  {179, 179, 179, 255}   // gris claro
#define COLOR_BAR_BG      { 60,  60,  60, 255}   // fondo barra progreso

#define SCREEN_W 1280
#define SCREEN_H 720

typedef struct {
    SDL_Renderer* renderer;
    TTF_Font*     font_regular;
    TTF_Font*     font_bold;
    TTF_Font*     font_small;
} RenderCtx;

// Inicialización
int  render_init(RenderCtx* ctx, SDL_Window* window);
void render_destroy(RenderCtx* ctx);

// Primitivas
void render_clear(RenderCtx* ctx);
void render_present(RenderCtx* ctx);
void render_rect(RenderCtx* ctx, int x, int y, int w, int h, SDL_Color color);
void render_rect_rounded(RenderCtx* ctx, int x, int y, int w, int h, int r, SDL_Color color);
void render_bar(RenderCtx* ctx, int x, int y, int w, int h, float pct, SDL_Color bg, SDL_Color fill);

// Texto
void render_text(RenderCtx* ctx, const char* text, int x, int y, SDL_Color color, TTF_Font* font);
void render_text_centered(RenderCtx* ctx, const char* text, int x, int y, int w, SDL_Color color, TTF_Font* font);

// Utilidad
void render_format_time(int secs, char* buf, int buf_size); // 183 → "3:03"