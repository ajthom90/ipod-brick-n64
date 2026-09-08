#ifndef MUSIC_H
#define MUSIC_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "game.h"

enum { MUSIC_CHANNELS = 4, MUSIC_MAX_EVENTS = 512 };
typedef enum { CH_PULSE1 = 0, CH_PULSE2, CH_TRIANGLE, CH_NOISE } music_channel_t;
typedef struct { uint8_t note; uint8_t len; } music_event_t;   /* note: 0 rest, 1..127 MIDI note, 250 K, 251 S, 252 H, 253 O; len in steps 1..255 */
typedef struct { uint8_t duty; uint8_t gain; uint8_t decay; uint8_t sustain; } music_chparms_t; /* duty 12/25/50, gain 0..100, decay steps, sustain 0..100 */
typedef struct {
    int bpm;
    int steps;                                   /* total steps per loop (equal on all channels) */
    music_chparms_t parms[MUSIC_CHANNELS];
    music_event_t events[MUSIC_CHANNELS][MUSIC_MAX_EVENTS];
    int nevents[MUSIC_CHANNELS];
} music_track_t;
typedef struct { const char *name; int bpm; const char *pattern[MUSIC_CHANNELS]; } music_src_t;

extern const music_src_t MUSIC_SRC[MUSIC_TRACK_COUNT];   /* src/music_data.c, indexed by music_track_id_t */

/* Parses one pattern string into a channel. Returns false and fills err (if errcap > 0) on bad tokens
 * or when a bar delimited by '|' does not contain exactly 16 steps. */
bool music_parse_channel(const char *pattern, music_chparms_t *parms, music_event_t *events, int *nevents, int *steps, char *err, size_t errcap);
/* Parses all four channels; fails if their step counts differ. */
bool music_parse(const music_src_t *src, music_track_t *out, char *err, size_t errcap);
/* MIDI note (0..127) to frequency in Q8.8 Hz. A4 (69) = 440 << 8. */
uint32_t music_note_freq_q8(int midi);

#endif
