#include "settings.h"
#include "credits.h"
#include "utils/lang.h"

enum {
    SETTINGS_LANGUAGE,
    SETTINGS_CREDITS,
    SETTINGS_BACK,
    SETTINGS_ITEMS
};

void settings_update(UIState* ui, u64 down, float delta) {
    if (down & HidNpadButton_B) {
        ui->active = SCREEN_LIBRARY;
        return;
    }

    if ((down & HidNpadButton_Up) || (down & HidNpadButton_StickLUp) ||
        (down & HidNpadButton_StickRUp)) {
        if (ui->settings_selected > 0) ui->settings_selected--;
    }
    if ((down & HidNpadButton_Down) || (down & HidNpadButton_StickLDown) ||
        (down & HidNpadButton_StickRDown)) {
        if (ui->settings_selected < SETTINGS_ITEMS - 1) ui->settings_selected++;
    }

    if (down & HidNpadButton_A) {
        switch (ui->settings_selected) {
            case SETTINGS_LANGUAGE: {
                Language next = lang_get_language() + 1;
                if (next >= LANG_COUNT) next = LANG_EN;
                lang_set_language(next);
                break;
            }
            case SETTINGS_CREDITS:
                ui->active = SCREEN_CREDITS;
                break;
            case SETTINGS_BACK:
                ui->active = SCREEN_LIBRARY;
                break;
        }
    }

    // Animación suave del highlight: sigue a la opción seleccionada.
    const int start_y = 200;
    const int gap = 70;
    const int highlight_h = 58;
    int target_y = start_y + ui->settings_selected * gap - (highlight_h - 46) / 2;
    float speed = 12.0f;
    ui->settings_highlight_y += (target_y - ui->settings_highlight_y) * speed * delta;
}

void settings_draw(UIState* ui, RenderCtx* ctx) {

    SDL_Color white = {255, 255, 255, 255};
    SDL_Color muted = {179, 179, 179, 255};
    // Verde m\u00e1s oscuro para que el texto blanco tenga buen contraste.
    SDL_Color green = {29,  185, 84,  255};
    SDL_Color surface = {24, 24, 24, 255};

    render_text_centered(ctx, lang_get(LK_SETTINGS), 0, 80, SCREEN_W, white, ctx->font_bold);

    const char* items[SETTINGS_ITEMS] = {
        [SETTINGS_LANGUAGE] = lang_get(LK_LANGUAGE),
        [SETTINGS_CREDITS]  = lang_get(LK_CREDITS),
        [SETTINGS_BACK]     = lang_get(LK_BACK),
    };

    int start_y = 200;
    int gap = 70;
    int highlight_h = 58;
    int highlight_w = 600;
    int highlight_x = (SCREEN_W - highlight_w) / 2;

    // Primera pasada: dibujar todos los fondos/bordes para evitar
    // colisiones de Z entre el highlight animado y los bordes est\u00e1ticos.
    for (int i = 0; i < SETTINGS_ITEMS; i++) {
        int y = start_y + i * gap;
        bool selected = (i == ui->settings_selected);
        if (selected) {
            render_rect_rounded(ctx, highlight_x, (int)ui->settings_highlight_y,
                highlight_w, highlight_h, 12, green);
        } else {
            // Borde sutil para las no seleccionadas.
            render_rect_rounded(ctx, highlight_x, y - (highlight_h - 46) / 2,
                highlight_w, highlight_h, 12, surface);
        }
    }

    // Segunda pasada: textos siempre encima de los fondos.
    for (int i = 0; i < SETTINGS_ITEMS; i++) {
        int y = start_y + i * gap;
        render_text_centered(ctx, items[i], 0, y, SCREEN_W, white, ctx->font_regular);

        // Al lado de idioma se muestra el idioma actual en blanco.
        if (i == SETTINGS_LANGUAGE) {
            render_text(ctx, lang_name(lang_get_language()),
                760, y, white, ctx->font_small);
        }
    }

    render_text_centered(ctx, "[A] Select  [B] Back",
        0, SCREEN_H - 80, SCREEN_W, muted, ctx->font_small);
}
