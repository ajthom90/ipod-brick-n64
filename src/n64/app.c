#include <libdragon.h>
#include "../brick.h"

#define FONT_ID 1

static color_t rgb(uint32_t c) {
    return RGBA32((c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF, 0xFF);
}

int main(void) {
    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 3, GAMMA_NONE, FILTERS_RESAMPLE);
    rdpq_init();
    joypad_init();

    rdpq_font_t *font = rdpq_font_load_builtin(FONT_BUILTIN_DEBUG_MONO);
    rdpq_font_style(font, 0, &(rdpq_fontstyle_t){ .color = rgb(BRICK_COLOR_TEXT) });
    rdpq_text_register_font(FONT_ID, font);

    brick_game_t game;
    brick_init(&game);

    while (1) {
        surface_t *fb = display_get();
        rdpq_attach(fb, NULL);
        rdpq_set_mode_fill(rgb(BRICK_COLOR_BG));
        rdpq_fill_rectangle(0, 0, BRICK_SCREEN_W, BRICK_SCREEN_H);
        rdpq_set_mode_standard();
        rdpq_textparms_t center = { .width = BRICK_SCREEN_W, .align = ALIGN_CENTER };
        rdpq_text_printf(&center, FONT_ID, 0, 110, "BRICK");
        rdpq_text_printf(&center, FONT_ID, 0, 140, "PRESS A");
        rdpq_detach_show();
        brick_update(&game, &(brick_input_t){0});
    }
}
