#include "credits.h"
#include "utils/lang.h"

void credits_update(UIState* ui, u64 down) {
    if (down & HidNpadButton_B) {
        ui->active = SCREEN_SETTINGS;
    }
}

void credits_draw(UIState* ui, RenderCtx* ctx) {
    (void)ui;

    SDL_Color white = {255, 255, 255, 255};
    SDL_Color muted = {179, 179, 179, 255};
    SDL_Color green = {30,  215, 96,  255};

    render_text_centered(ctx, lang_get(LK_CREDITS), 0, 70, SCREEN_W, white, ctx->font_bold);

    const char* lines[] = {
        "MangoSpot",
        "",
        lang_get(LK_CREDITS_MADE_BY),
        lang_get(LK_CREDITS_WITH_LOVE),
        lang_get(LK_CREDITS_GITHUB),
        "",
        lang_get(LK_CREDITS_REPOS),
        "",
        lang_get(LK_CREDITS_INSPIRATION),
        "github.com/feelfreelinux/cspot",
        "github.com/devkitPro/libnx",
        "libsdl.org",
    };

    int line_count = sizeof(lines) / sizeof(lines[0]);
    int start_y = 160;
    int gap = 40;
    for (int i = 0; i < line_count; i++) {
        SDL_Color color = (i == 0) ? green : white;
        TTF_Font* font = (i == 0) ? ctx->font_bold : ctx->font_regular;
        render_text_centered(ctx, lines[i], 0, start_y + i * gap, SCREEN_W, color, font);
    }

    render_text_centered(ctx, lang_get(LK_BACK),
        0, SCREEN_H - 80, SCREEN_W, muted, ctx->font_small);
}
