#include "render.h"
#include <stdio.h>
#include <string.h>

int render_init(RenderCtx* ctx, SDL_Window* window) {
    FILE* log = fopen("sdmc:/switch/spotiswitch/debug.log", "a");

    ctx->renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (!ctx->renderer) {
        fprintf(log, "renderer fallo: %s\n", SDL_GetError());
        fclose(log);
        return -1;
    }
    fprintf(log, "renderer OK\n"); fflush(log);

    if (TTF_Init() < 0) {
        fprintf(log, "TTF_Init fallo: %s\n", TTF_GetError());
        fclose(log);
        return -1;
    }
    fprintf(log, "TTF OK\n"); fflush(log);

    const char* path_regular = "sdmc:/switch/spotiswitch/fonts/font.ttf";
    const char* path_bold    = "sdmc:/switch/spotiswitch/fonts/font-bold.ttf";

    fprintf(log, "intentando abrir: %s\n", path_regular); fflush(log);
    ctx->font_regular = TTF_OpenFont(path_regular, 24);
    fprintf(log, "font_regular: %s\n", ctx->font_regular ? "OK" : TTF_GetError()); fflush(log);

    fprintf(log, "intentando abrir: %s\n", path_bold); fflush(log);
    ctx->font_bold = TTF_OpenFont(path_bold, 24);
    fprintf(log, "font_bold: %s\n", ctx->font_bold ? "OK" : TTF_GetError()); fflush(log);

    ctx->font_small = TTF_OpenFont(path_regular, 18);
    fprintf(log, "font_small: %s\n", ctx->font_small ? "OK" : TTF_GetError()); fflush(log);

    if (!ctx->font_regular || !ctx->font_bold || !ctx->font_small) {
        fprintf(log, "una o mas fuentes fallaron\n");
        fclose(log);
        return -1;
    }

    fprintf(log, "render_init completo\n");
    fclose(log);
    return 0;
}

void render_destroy(RenderCtx* ctx) {
    TTF_CloseFont(ctx->font_regular);
    TTF_CloseFont(ctx->font_bold);
    TTF_CloseFont(ctx->font_small);
    TTF_Quit();
    SDL_DestroyRenderer(ctx->renderer);
}

void render_clear(RenderCtx* ctx) {
    SDL_SetRenderDrawColor(ctx->renderer, 12, 12, 12, 255);
    SDL_RenderClear(ctx->renderer);
}

void render_present(RenderCtx* ctx) {
    SDL_RenderPresent(ctx->renderer);
}

void render_rect(RenderCtx* ctx, int x, int y, int w, int h, SDL_Color c) {
    SDL_SetRenderDrawColor(ctx->renderer, c.r, c.g, c.b, c.a);
    SDL_Rect rect = { x, y, w, h };
    SDL_RenderFillRect(ctx->renderer, &rect);
}

void render_rect_rounded(RenderCtx* ctx, int x, int y, int w, int h, int r, SDL_Color c) {
    SDL_SetRenderDrawColor(ctx->renderer, c.r, c.g, c.b, c.a);

    SDL_Rect rects[3] = {
        { x + r, y,     w - 2*r, h     },
        { x,     y + r, r,       h-2*r },
        { x+w-r, y + r, r,       h-2*r },
    };
    SDL_RenderFillRects(ctx->renderer, rects, 3);

    int cx[4] = { x+r,   x+w-r, x+r,   x+w-r };
    int cy[4] = { y+r,   y+r,   y+h-r, y+h-r };
    for (int i = 0; i < 4; i++) {
        for (int dy = -r; dy <= r; dy++) {
            for (int dx = -r; dx <= r; dx++) {
                if (dx*dx + dy*dy <= r*r) {
                    SDL_RenderDrawPoint(ctx->renderer, cx[i]+dx, cy[i]+dy);
                }
            }
        }
    }
}

void render_bar(RenderCtx* ctx, int x, int y, int w, int h,
                float pct, SDL_Color bg, SDL_Color fill) {
    render_rect_rounded(ctx, x, y, w, h, h/2, bg);
    int fill_w = (int)(w * pct);
    if (fill_w > 0)
        render_rect_rounded(ctx, x, y, fill_w, h, h/2, fill);

    if (fill_w > 0) {
        SDL_SetRenderDrawColor(ctx->renderer, 255, 255, 255, 255);
        int cx = x + fill_w;
        int cy = y + h/2;
        int rr = 6;
        for (int dy = -rr; dy <= rr; dy++)
            for (int dx = -rr; dx <= rr; dx++)
                if (dx*dx + dy*dy <= rr*rr)
                    SDL_RenderDrawPoint(ctx->renderer, cx+dx, cy+dy);
    }
}

void render_text(RenderCtx* ctx, const char* text, int x, int y,
                 SDL_Color color, TTF_Font* font) {
    if (!text || !font) return;
    SDL_Surface* surf = TTF_RenderUTF8_Blended(font, text, color);
    if (!surf) return;
    SDL_Texture* tex = SDL_CreateTextureFromSurface(ctx->renderer, surf);
    SDL_Rect dst = { x, y, surf->w, surf->h };
    SDL_RenderCopy(ctx->renderer, tex, NULL, &dst);
    SDL_DestroyTexture(tex);
    SDL_FreeSurface(surf);
}

void render_text_centered(RenderCtx* ctx, const char* text, int x, int y,
                          int w, SDL_Color color, TTF_Font* font) {
    if (!text || !font) return;
    int tw, th;
    TTF_SizeUTF8(font, text, &tw, &th);
    render_text(ctx, text, x + (w - tw) / 2, y, color, font);
}

void render_format_time(int secs, char* buf, int buf_size) {
    int m = secs / 60;
    int s = secs % 60;
    snprintf(buf, buf_size, "%d:%02d", m, s);
}