#include "music.h"
#include <string.h>

enum { NOTE_K = 250, NOTE_S = 251, NOTE_H = 252, NOTE_O = 253 };

/* Octave-4 frequencies in Q8.8 Hz. A4 = 440 << 8 = 112640. */
static const uint32_t FREQ_Q8_OCT4[12] = {
    66977u, 70959u, 75178u, 79649u, 84385u, 89403u,
    94719u, 100351u, 106318u, 112640u, 119338u, 126445u
};

uint32_t music_note_freq_q8(int midi) {
    if (midi < 0) midi = 0;
    if (midi > 127) midi = 127;
    int octave = midi / 12 - 1;
    int semi = midi % 12;
    uint32_t f = FREQ_Q8_OCT4[semi];
    if (octave >= 4) {
        unsigned s = (unsigned)(octave - 4);
        if (s > 14u) s = 14u;
        f <<= s;
    } else {
        f >>= (unsigned)(4 - octave);
    }
    return f;
}

static void set_err(char *err, size_t cap, const char *msg) {
    if (!err || cap == 0) return;
    size_t n = 0;
    while (msg[n] && n + 1 < cap) {
        err[n] = msg[n];
        n++;
    }
    err[n] = '\0';
}

static int is_ws(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

static int starts_with(const char *s, const char *pfx) {
    while (*pfx) {
        if (*s != *pfx) return 0;
        s++;
        pfx++;
    }
    return 1;
}

static int parse_int(const char *s, const char **end, int *ok) {
    int v = 0;
    int any = 0;
    while (*s >= '0' && *s <= '9') {
        if (v > 100000000) {
            *ok = 0;
            *end = s;
            return 0;
        }
        v = v * 10 + (*s - '0');
        s++;
        any = 1;
    }
    *end = s;
    *ok = any;
    return v;
}

static int parse_len(const char *s, int *len) {
    if (*s == '\0') {
        *len = 1;
        return 1;
    }
    if (*s != ':') return 0;
    s++;
    int ok = 0;
    int v = parse_int(s, &s, &ok);
    if (!ok || *s != '\0' || v < 1 || v > 255) return 0;
    *len = v;
    return 1;
}

static int push_event(music_event_t *events, int *nevents, int *steps, int *bar_steps,
                      uint8_t note, int len, char *err, size_t errcap) {
    if (*nevents >= MUSIC_MAX_EVENTS) {
        set_err(err, errcap, "too many events");
        return 0;
    }
    events[*nevents].note = note;
    events[*nevents].len = (uint8_t)len;
    (*nevents)++;
    *steps += len;
    *bar_steps += len;
    return 1;
}

bool music_parse_channel(const char *pattern, music_chparms_t *parms, music_event_t *events,
                         int *nevents, int *steps, char *err, size_t errcap) {
    if (!pattern || !parms || !events || !nevents || !steps) {
        set_err(err, errcap, "null argument");
        return false;
    }
    parms->duty = 50;
    parms->gain = 60;
    parms->decay = 0;
    parms->sustain = 100;
    *nevents = 0;
    *steps = 0;
    int bar_steps = 0;
    int seen_event = 0;
    const char *p = pattern;

    while (*p) {
        while (*p && is_ws(*p)) p++;
        if (!*p) break;

        char tok[64];
        int n = 0;
        while (*p && !is_ws(*p)) {
            if (n < 63) tok[n++] = *p;
            p++;
        }
        tok[n] = '\0';
        if (n == 0) continue;

        if (tok[0] == '|' && tok[1] == '\0') {
            if (bar_steps != 16) {
                set_err(err, errcap, "bar does not contain exactly 16 steps");
                return false;
            }
            bar_steps = 0;
            continue;
        }

        int is_header = starts_with(tok, "duty=") || starts_with(tok, "gain=") ||
                        starts_with(tok, "decay=") || starts_with(tok, "sustain=");
        if (is_header) {
            if (seen_event) {
                set_err(err, errcap, "header after note");
                return false;
            }
            const char *eq = tok;
            while (*eq && *eq != '=') eq++;
            if (*eq != '=') {
                set_err(err, errcap, "bad header");
                return false;
            }
            int ok = 0;
            const char *end = eq + 1;
            int v = parse_int(eq + 1, &end, &ok);
            if (!ok || *end != '\0' || v < 0 || v > 255) {
                set_err(err, errcap, "bad header value");
                return false;
            }
            if (starts_with(tok, "duty=")) {
                if (v != 12 && v != 25 && v != 50) {
                    set_err(err, errcap, "bad duty");
                    return false;
                }
                parms->duty = (uint8_t)v;
            } else if (starts_with(tok, "gain=")) {
                parms->gain = (uint8_t)v;
            } else if (starts_with(tok, "decay=")) {
                parms->decay = (uint8_t)v;
            } else {
                parms->sustain = (uint8_t)v;
            }
            continue;
        }

        if (tok[0] == '-') {
            int len = 1;
            if (!parse_len(tok + 1, &len)) {
                set_err(err, errcap, "unknown token");
                return false;
            }
            if (!push_event(events, nevents, steps, &bar_steps, 0, len, err, errcap)) return false;
            seen_event = 1;
            continue;
        }

        if ((tok[0] == 'K' || tok[0] == 'S' || tok[0] == 'H' || tok[0] == 'O') &&
            (tok[1] == '\0' || tok[1] == ':')) {
            uint8_t note = NOTE_K;
            if (tok[0] == 'S') note = NOTE_S;
            else if (tok[0] == 'H') note = NOTE_H;
            else if (tok[0] == 'O') note = NOTE_O;
            int len = 1;
            if (!parse_len(tok + 1, &len)) {
                set_err(err, errcap, "unknown token");
                return false;
            }
            if (!push_event(events, nevents, steps, &bar_steps, note, len, err, errcap)) return false;
            seen_event = 1;
            continue;
        }

        if (tok[0] >= 'A' && tok[0] <= 'G') {
            static const int letter_semi[7] = { 9, 11, 0, 2, 4, 5, 7 };
            int semi = letter_semi[tok[0] - 'A'];
            int i = 1;
            if (tok[i] == '#') {
                semi++;
                i++;
            }
            if (tok[i] < '0' || tok[i] > '8') {
                set_err(err, errcap, "unknown token");
                return false;
            }
            int oct = tok[i] - '0';
            i++;
            if (semi > 11) {
                semi -= 12;
                oct++;
            }
            int len = 1;
            if (!parse_len(tok + i, &len)) {
                set_err(err, errcap, "unknown token");
                return false;
            }
            int midi = 12 * (oct + 1) + semi;
            if (midi < 0) midi = 0;
            if (midi > 127) midi = 127;
            if (!push_event(events, nevents, steps, &bar_steps, (uint8_t)midi, len, err, errcap)) return false;
            seen_event = 1;
            continue;
        }

        set_err(err, errcap, "unknown token");
        return false;
    }

    if (bar_steps != 0 && bar_steps != 16) {
        set_err(err, errcap, "bar does not contain exactly 16 steps");
        return false;
    }
    return true;
}

bool music_parse(const music_src_t *src, music_track_t *out, char *err, size_t errcap) {
    if (!src || !out) {
        set_err(err, errcap, "null argument");
        return false;
    }
    memset(out, 0, sizeof *out);
    out->bpm = src->bpm;
    int steps0 = -1;
    for (int ch = 0; ch < MUSIC_CHANNELS; ch++) {
        const char *pat = src->pattern[ch] ? src->pattern[ch] : "";
        int steps = 0;
        if (!music_parse_channel(pat, &out->parms[ch], out->events[ch],
                                 &out->nevents[ch], &steps, err, errcap)) {
            return false;
        }
        if (ch == 0) steps0 = steps;
        else if (steps != steps0) {
            set_err(err, errcap, "channel step counts differ");
            return false;
        }
    }
    out->steps = steps0;
    return true;
}
