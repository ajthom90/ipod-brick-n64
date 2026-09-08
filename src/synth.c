#include "synth.h"
#include <string.h>

enum { ENV_FULL = 65536, MIX_SCALE = 110, SFX_AMP = 8000 };
enum { NOTE_K = 250, NOTE_S = 251, NOTE_H = 252, NOTE_O = 253 };

static uint32_t duty_thresh(uint8_t duty) {
    if (duty == 12) return 8192u;
    if (duty == 25) return 16384u;
    return 32768u;
}

static uint32_t freq_to_inc(uint32_t freq_q8, int sample_rate) {
    if (sample_rate <= 0) return 0;
    return (uint32_t)(((uint64_t)freq_q8 << 8) / (uint32_t)sample_rate);
}

static int32_t clip16(int32_t acc) {
    if (acc > 32767) return 32767;
    if (acc < -32768) return -32768;
    return acc;
}

static void pulse_off(synth_pulse_t *p) {
    p->on = 0;
    p->vol = 0;
    p->inc = 0;
    p->env_left = 0;
}

static void pulse_on(synth_pulse_t *p, uint32_t inc, int decay_steps, int sustain, int step_samples) {
    p->inc = inc;
    p->phase = 0;
    p->on = 1;
    p->env_from = ENV_FULL;
    p->vol = ENV_FULL;
    p->target = (int)((int64_t)ENV_FULL * sustain / 100);
    if (p->target < 0) p->target = 0;
    if (p->target > ENV_FULL) p->target = ENV_FULL;
    if (decay_steps <= 0 || step_samples <= 0) {
        p->vol = p->target;
        p->env_left = 0;
        p->env_total = 0;
        p->decay_per_sample = 0;
        return;
    }
    p->env_total = decay_steps * step_samples;
    p->env_left = p->env_total;
    p->decay_per_sample = (ENV_FULL - p->target) / p->env_total;
}

static void pulse_tick_env(synth_pulse_t *p) {
    if (!p->on) {
        p->vol = 0;
        return;
    }
    if (p->env_left > 0) {
        int gone = p->env_total - p->env_left;
        p->vol = p->env_from + (int)((int64_t)(p->target - p->env_from) * gone / p->env_total);
        p->env_left--;
        if (p->env_left == 0) p->vol = p->target;
    } else {
        p->vol = p->target;
    }
}

static uint16_t lfsr_clock(uint16_t r, uint8_t tap) {
    uint16_t bit = (uint16_t)((r ^ (r >> tap)) & 1u);
    r = (uint16_t)((r >> 1) | (bit << 14));
    return (uint16_t)(r & 0x7FFFu);
}

static void trig_drum(synth_t *s, int kind, int ms_override) {
    int tap, period, ms;
    if (kind == 0) { tap = 1; period = 16; ms = 90; }
    else if (kind == 1) { tap = 1; period = 4; ms = 120; }
    else if (kind == 2) { tap = 6; period = 1; ms = 30; }
    else { tap = 6; period = 1; ms = 150; }
    if (ms_override > 0) ms = ms_override;
    s->noise_tap = (uint8_t)tap;
    s->noise_period = period;
    s->noise_count = 0;
    s->noise_vol = ENV_FULL;
    s->noise_on = 1;
    int samples = s->sample_rate * ms / 1000;
    if (samples < 1) samples = 1;
    s->noise_decay = ENV_FULL / samples;
    if (s->noise_decay < 1) s->noise_decay = 1;
}

static void silence_music_voices(synth_t *s) {
    pulse_off(&s->pulse[0]);
    pulse_off(&s->pulse[1]);
    s->tri_on = 0;
    s->tri_inc = 0;
    s->noise_on = 0;
    s->noise_vol = 0;
}

static void channel_start_event(synth_t *s, int ch, const music_event_t *ev) {
    const music_chparms_t *p = &s->track->parms[ch];
    if (ch == CH_PULSE1 || ch == CH_PULSE2) {
        synth_pulse_t *pl = &s->pulse[ch];
        pl->duty = p->duty;
        pl->gain = p->gain;
        if (ev->note == 0) {
            pulse_off(pl);
        } else {
            uint32_t inc = freq_to_inc(music_note_freq_q8(ev->note), s->sample_rate);
            pulse_on(pl, inc, p->decay, p->sustain, s->step_samples);
        }
        return;
    }
    if (ch == CH_TRIANGLE) {
        s->tri_gain = p->gain;
        if (ev->note == 0) {
            s->tri_on = 0;
            s->tri_inc = 0;
        } else {
            s->tri_inc = freq_to_inc(music_note_freq_q8(ev->note), s->sample_rate);
            s->tri_on = 1;
            s->tri_phase = 0;
        }
        return;
    }
    s->noise_gain = p->gain;
    if (ev->note == NOTE_K) trig_drum(s, 0, 0);
    else if (ev->note == NOTE_S) trig_drum(s, 1, 0);
    else if (ev->note == NOTE_H) trig_drum(s, 2, 0);
    else if (ev->note == NOTE_O) trig_drum(s, 3, 0);
}

static void seq_begin_loop(synth_t *s) {
    s->step_index = 0;
    s->step_pos = 0;
    for (int ch = 0; ch < MUSIC_CHANNELS; ch++) {
        s->ev_index[ch] = 0;
        if (s->track->nevents[ch] > 0) {
            s->ev_left[ch] = s->track->events[ch][0].len;
            channel_start_event(s, ch, &s->track->events[ch][0]);
        } else {
            s->ev_left[ch] = 0;
        }
    }
}

static void seq_advance_step(synth_t *s) {
    s->step_index++;
    if (s->step_index >= s->track->steps) {
        seq_begin_loop(s);
        return;
    }
    for (int ch = 0; ch < MUSIC_CHANNELS; ch++) {
        if (s->track->nevents[ch] <= 0) continue;
        s->ev_left[ch]--;
        if (s->ev_left[ch] <= 0) {
            s->ev_index[ch]++;
            if (s->ev_index[ch] >= s->track->nevents[ch]) s->ev_index[ch] = 0;
            int ei = s->ev_index[ch];
            s->ev_left[ch] = s->track->events[ch][ei].len;
            channel_start_event(s, ch, &s->track->events[ch][ei]);
        }
    }
}

void synth_init(synth_t *s, int sample_rate) {
    memset(s, 0, sizeof *s);
    s->sample_rate = sample_rate > 0 ? sample_rate : 22050;
    s->music_on = true;
    s->sfx_on = true;
    s->volume = 10;
    s->noise_lfsr = 1;
    s->sfx_n_lfsr = 1;
    s->pulse[0].duty = 50;
    s->pulse[1].duty = 50;
    s->sfx_duty = 50;
}

void synth_set_track(synth_t *s, const music_track_t *t) {
    silence_music_voices(s);
    s->track = t;
    s->step_samples = 0;
    s->step_pos = 0;
    s->step_index = 0;
    if (!t || t->bpm <= 0 || t->steps <= 0) return;
    s->step_samples = (int)((int64_t)s->sample_rate * 15 / t->bpm);
    if (s->step_samples < 1) s->step_samples = 1;
    seq_begin_loop(s);
}

void synth_set_volume(synth_t *s, int vol) {
    if (vol < 0) vol = 0;
    if (vol > 10) vol = 10;
    s->volume = vol;
}

void synth_set_music_enabled(synth_t *s, bool on) { s->music_on = on; }
void synth_set_sfx_enabled(synth_t *s, bool on) { s->sfx_on = on; }

typedef struct {
    int nnotes;
    int f0[4];
    int f1;
    int ms;
    int duty;
    int noise;     /* 0 none, 1 H, 2 K */
    int noise_ms;
} sfx_preset_t;

static const sfx_preset_t SFX_PRESET[SFX_COUNT] = {
    { 0, { 0 }, 0, 0, 50, 0, 0 },
    { 1, { 880 }, 660, 50, 50, 0, 0 },
    { 1, { 1320 }, 1320, 40, 25, 0, 0 },
    { 3, { 523, 659, 784 }, 0, 60, 50, 0, 0 },
    { 2, { 660, 990 }, 0, 50, 50, 0, 0 },
    { 1, { 200 }, 100, 60, 12, 1, 30 },
    { 1, { 120 }, 40, 200, 12, 2, 200 },
    { 1, { 1047 }, 1047, 120, 50, 0, 0 },
    { 1, { 440 }, 880, 80, 25, 0, 0 },
    { 4, { 523, 440, 349, 262 }, 0, 150, 50, 0, 0 },
    { 1, { 1500 }, 1500, 20, 12, 0, 0 },
    { 1, { 880 }, 1320, 60, 50, 0, 0 },
};

static void sfx_start_note(synth_t *s, int f0_hz, int f1_hz, int samples) {
    if (samples < 1) samples = 1;
    s->sfx_f0_q8 = f0_hz << 8;
    s->sfx_f1_q8 = f1_hz << 8;
    s->sfx_sweep_len = samples;
    s->sfx_sweep_pos = 0;
    s->sfx_left = samples;
    s->sfx_phase = 0;
    s->sfx_inc = freq_to_inc((uint32_t)s->sfx_f0_q8, s->sample_rate);
}

void synth_play_sfx(synth_t *s, sfx_id_t id) {
    if (id <= SFX_NONE || id >= SFX_COUNT) return;
    const sfx_preset_t *p = &SFX_PRESET[id];
    s->sfx_active = 1;
    s->sfx_nnotes = p->nnotes;
    s->sfx_note_i = 0;
    for (int i = 0; i < 4; i++) s->sfx_note_hz[i] = p->f0[i];
    s->sfx_duty = (uint8_t)p->duty;
    int dur = s->sample_rate * p->ms / 1000;
    if (dur < 1) dur = 1;
    s->sfx_note_dur = dur;
    if (p->nnotes > 1) sfx_start_note(s, p->f0[0], p->f0[0], dur);
    else sfx_start_note(s, p->f0[0], p->f1, dur);

    if (p->noise) {
        int tap = (p->noise == 2) ? 1 : 6;
        int period = (p->noise == 2) ? 16 : 1;
        s->sfx_n_tap = (uint8_t)tap;
        s->sfx_n_period = period;
        s->sfx_n_count = 0;
        s->sfx_n_vol = ENV_FULL;
        s->sfx_n_lfsr = 1;
        int ns = s->sample_rate * p->noise_ms / 1000;
        if (ns < 1) ns = 1;
        s->sfx_n_decay = ENV_FULL / ns;
        if (s->sfx_n_decay < 1) s->sfx_n_decay = 1;
    } else {
        s->sfx_n_vol = 0;
        s->sfx_n_decay = 0;
    }
}

static int pulse_contrib(synth_pulse_t *p) {
    if (!p->on || p->vol <= 0) {
        pulse_tick_env(p);
        return 0;
    }
    int w = ((p->phase & 0xFFFFu) < duty_thresh(p->duty)) ? 1 : -1;
    p->phase += p->inc;
    int contrib = (int)((int64_t)w * p->gain * p->vol / ENV_FULL);
    pulse_tick_env(p);
    return contrib;
}

static int tri_contrib(synth_t *s) {
    if (!s->tri_on) return 0;
    uint32_t step = (s->tri_phase >> 11) & 31u;
    int amp = (step < 16u) ? (15 - (int)step) : ((int)step - 16);
    int signed_tri = amp * 2 - 15;
    s->tri_phase += s->tri_inc;
    return (int)((int64_t)signed_tri * s->tri_gain / 15);
}

static int noise_contrib(synth_t *s) {
    if (s->noise_vol <= 0) {
        s->noise_on = 0;
        return 0;
    }
    int w = (s->noise_lfsr & 1u) ? 1 : -1;
    int contrib = (int)((int64_t)w * s->noise_gain * s->noise_vol / ENV_FULL);
    s->noise_count++;
    if (s->noise_count >= s->noise_period) {
        s->noise_count = 0;
        s->noise_lfsr = lfsr_clock(s->noise_lfsr, s->noise_tap);
        if (s->noise_lfsr == 0) s->noise_lfsr = 1;
    }
    s->noise_vol -= s->noise_decay;
    if (s->noise_vol < 0) s->noise_vol = 0;
    return contrib;
}

static int sfx_contrib(synth_t *s) {
    int acc = 0;
    if (s->sfx_active) {
        int sp = s->sfx_sweep_pos;
        if (sp > s->sfx_sweep_len) sp = s->sfx_sweep_len;
        int32_t fq = s->sfx_f0_q8 +
            (int32_t)((int64_t)(s->sfx_f1_q8 - s->sfx_f0_q8) * sp / s->sfx_sweep_len);
        if (fq < 0) fq = 0;
        s->sfx_inc = freq_to_inc((uint32_t)fq, s->sample_rate);
        int w = ((s->sfx_phase & 0xFFFFu) < duty_thresh(s->sfx_duty)) ? 1 : -1;
        s->sfx_phase += s->sfx_inc;
        acc += w * SFX_AMP;
        s->sfx_sweep_pos++;
        s->sfx_left--;
        if (s->sfx_left <= 0) {
            s->sfx_note_i++;
            if (s->sfx_note_i < s->sfx_nnotes) {
                int hz = s->sfx_note_hz[s->sfx_note_i];
                sfx_start_note(s, hz, hz, s->sfx_note_dur);
            } else {
                s->sfx_active = 0;
            }
        }
    }
    if (s->sfx_n_vol > 0) {
        int w = (s->sfx_n_lfsr & 1u) ? 1 : -1;
        acc += (int)((int64_t)w * SFX_AMP * s->sfx_n_vol / ENV_FULL);
        s->sfx_n_count++;
        if (s->sfx_n_count >= s->sfx_n_period) {
            s->sfx_n_count = 0;
            s->sfx_n_lfsr = lfsr_clock(s->sfx_n_lfsr, s->sfx_n_tap);
            if (s->sfx_n_lfsr == 0) s->sfx_n_lfsr = 1;
        }
        s->sfx_n_vol -= s->sfx_n_decay;
        if (s->sfx_n_vol < 0) s->sfx_n_vol = 0;
    }
    return acc;
}

void synth_render(synth_t *s, int16_t *out, int nframes) {
    if (!out || nframes <= 0) return;
    for (int i = 0; i < nframes; i++) {
        int32_t acc = 0;
        if (s->music_on && s->track) {
            int mix = pulse_contrib(&s->pulse[0]) + pulse_contrib(&s->pulse[1]) +
                      tri_contrib(s) + noise_contrib(s);
            acc += mix * MIX_SCALE;
        } else if (s->track) {
            pulse_contrib(&s->pulse[0]);
            pulse_contrib(&s->pulse[1]);
            tri_contrib(s);
            noise_contrib(s);
        }
        if (s->sfx_on) acc += sfx_contrib(s);
        else {
            int ignore = sfx_contrib(s);
            (void)ignore;
        }
        acc = acc * s->volume / 10;
        out[i] = (int16_t)clip16(acc);
        if (s->track && s->step_samples > 0) {
            s->step_pos++;
            if (s->step_pos >= s->step_samples) {
                s->step_pos = 0;
                seq_advance_step(s);
            }
        }
    }
}
