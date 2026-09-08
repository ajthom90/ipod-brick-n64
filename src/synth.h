#ifndef SYNTH_H
#define SYNTH_H

#include <stdint.h>
#include <stdbool.h>
#include "music.h"
#include "sfx.h"

typedef struct {
    uint32_t phase, inc;
    uint8_t duty;
    int vol;
    int target;
    int decay_per_sample;
    int env_from;
    int env_left;
    int env_total;
    int gain;
    int on;
} synth_pulse_t;

typedef struct synth_s {
    int sample_rate;
    const music_track_t *track; int step_samples; int step_pos; int step_index; int ev_index[4]; int ev_left[4];

    synth_pulse_t pulse[2];
    uint32_t tri_phase, tri_inc;
    int tri_gain, tri_on;

    uint16_t noise_lfsr;
    uint8_t noise_tap;
    int noise_period, noise_count, noise_vol, noise_decay, noise_gain, noise_on;

    int sfx_active;
    uint32_t sfx_phase, sfx_inc;
    uint8_t sfx_duty;
    int sfx_left;
    int sfx_f0_q8, sfx_f1_q8;
    int sfx_sweep_len, sfx_sweep_pos;
    int sfx_note_i, sfx_nnotes, sfx_note_hz[4], sfx_note_dur;
    uint16_t sfx_n_lfsr;
    uint8_t sfx_n_tap;
    int sfx_n_period, sfx_n_count, sfx_n_vol, sfx_n_decay;

    bool music_on, sfx_on; int volume;   /* volume 0..10 */
} synth_t;

void synth_init(synth_t *s, int sample_rate);
void synth_set_track(synth_t *s, const music_track_t *t);   /* NULL silences music; restarts from step 0 */
void synth_set_volume(synth_t *s, int vol);                 /* 0..10 */
void synth_set_music_enabled(synth_t *s, bool on);
void synth_set_sfx_enabled(synth_t *s, bool on);
void synth_play_sfx(synth_t *s, sfx_id_t id);
void synth_render(synth_t *s, int16_t *out, int nframes);   /* mono, signed 16-bit */

#endif
