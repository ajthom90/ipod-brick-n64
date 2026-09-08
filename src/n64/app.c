#include <libdragon.h>
#include "../brick.h"

#define FONT_ID 1
#define STICK_DEAD_ZONE 8
#define STICK_FULL 80

static color_t rgb(uint32_t c) {
    return RGBA32((c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF, 0xFF);
}

static void fill(brick_rect_t r) {
    rdpq_fill_rectangle(r.x0, r.y0, r.x1, r.y1);
}

#ifndef BRICK_AUTOPLAY
/* Sample the controller once per rendered frame. Axis/direction are levels;
 * launch/confirm/pause are OR-ed in so a press is never lost, and the caller
 * clears them after a tick consumes them. */
static void read_input(brick_input_t *in) {
    joypad_poll();
    joypad_inputs_t inputs = joypad_get_inputs(JOYPAD_PORT_1);
    joypad_buttons_t held = joypad_get_buttons(JOYPAD_PORT_1);
    joypad_buttons_t pressed = joypad_get_buttons_pressed(JOYPAD_PORT_1);

    int sx = inputs.stick_x;
    if (sx > -STICK_DEAD_ZONE && sx < STICK_DEAD_ZONE) sx = 0;
    if (sx > STICK_FULL) sx = STICK_FULL;
    if (sx < -STICK_FULL) sx = -STICK_FULL;
    in->paddle_axis = (int16_t)((sx * 256) / STICK_FULL);

    if (held.d_right || held.c_right) in->paddle_dir = 1;
    else if (held.d_left || held.c_left) in->paddle_dir = -1;
    else in->paddle_dir = 0;

    if (pressed.a) in->launch = true;
    if (pressed.a || pressed.b) in->confirm = true;
    if (pressed.start) in->pause = true;
}
#endif

static void render(const brick_game_t *g) {
    surface_t *fb = display_get();
    rdpq_attach(fb, NULL);

    rdpq_set_mode_fill(rgb(BRICK_COLOR_BG));
    rdpq_fill_rectangle(0, 0, BRICK_SCREEN_W, BRICK_SCREEN_H);

    if (g->state != BRICK_ST_TITLE) {
        for (int r = 0; r < BRICK_ROWS; r++) {
            rdpq_set_fill_color(rgb(brick_row_color(r)));
            for (int c = 0; c < BRICK_COLS; c++)
                if (g->cells[r][c]) fill(brick_cell_rect(r, c));
        }
        rdpq_set_fill_color(rgb(BRICK_COLOR_PADDLE));
        fill(brick_paddle_rect(g));
        rdpq_set_fill_color(rgb(BRICK_COLOR_BALL));
        fill(brick_ball_rect(g));
    }

    rdpq_set_mode_standard();
    rdpq_textparms_t left = { .width = 0, .align = ALIGN_LEFT };
    rdpq_textparms_t center = { .width = BRICK_SCREEN_W, .align = ALIGN_CENTER };
    rdpq_textparms_t right = { .width = BRICK_PLAY_X1, .align = ALIGN_RIGHT };

    if (g->state == BRICK_ST_TITLE) {
        rdpq_text_printf(&center, FONT_ID, 0, 100, "BRICK");
        rdpq_text_printf(&center, FONT_ID, 0, 130, "HIGH SCORE %d", g->high_score);
        rdpq_text_printf(&center, FONT_ID, 0, 160, "PRESS A");
    } else {
        rdpq_text_printf(&left, FONT_ID, BRICK_PLAY_X0, 16, "SCORE %d", g->score);
        rdpq_text_printf(&center, FONT_ID, 0, 16, "LIVES %d", g->lives);
        rdpq_text_printf(&right, FONT_ID, 0, 16, "LV %d", g->level);
        if (g->state == BRICK_ST_PAUSE) {
            rdpq_text_printf(&center, FONT_ID, 0, 160, "PAUSED");
        } else if (g->state == BRICK_ST_GAMEOVER) {
            rdpq_text_printf(&center, FONT_ID, 0, 150, "GAME OVER");
            rdpq_text_printf(&center, FONT_ID, 0, 170, "HIGH SCORE %d", g->high_score);
            rdpq_text_printf(&center, FONT_ID, 0, 190, "PRESS A");
        }
    }

    rdpq_detach_show();
}

int main(void) {
    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 3, GAMMA_NONE, FILTERS_RESAMPLE);
    rdpq_init();
#ifdef BRICK_DEBUG
    rdpq_debug_start();
#endif
    joypad_init();

    rdpq_font_t *font = rdpq_font_load_builtin(FONT_BUILTIN_DEBUG_MONO);
    rdpq_font_style(font, 0, &(rdpq_fontstyle_t){ .color = rgb(BRICK_COLOR_TEXT) });
    rdpq_text_register_font(FONT_ID, font);

    brick_game_t game;
    brick_init(&game);
    brick_input_t in = {0};

    const int hz = (get_tv_type() == TV_PAL) ? 50 : 60;
    const uint64_t dt = TICKS_PER_SECOND / hz;
    uint64_t prev = get_ticks();
    uint64_t acc = 0;

    while (1) {
        uint64_t now = get_ticks();
        acc += now - prev;
        prev = now;
#ifndef BRICK_AUTOPLAY
        read_input(&in);
#endif
        int steps = 0;
        while (acc >= dt && steps < BRICK_CATCHUP_MAX) {
#ifdef BRICK_AUTOPLAY
            brick_autoplay_input(&game, &in);
#endif
            brick_update(&game, &in);
            in.launch = in.confirm = in.pause = false;
            acc -= dt;
            steps++;
        }
        if (steps == BRICK_CATCHUP_MAX) acc = 0;   /* drop the backlog after a stall */
        render(&game);
    }
}
