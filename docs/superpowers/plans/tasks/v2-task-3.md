# v2 Task 3: Chiptune synth, notation sequencer, sound effects, seven tunes

You are implementing **Task 3 of the v2 plan** for a Nintendo 64 games collection built with libdragon. Tasks 1 and 2 are committed. This task is host-only: nothing in it touches the N64 adapter. You have no other context: read the files below first.

## Read first (in this order)

1. `docs/superpowers/plans/2026-09-08-retro-collection.md` — read the header, "Global Constraints", "Shared code", and all of **Task 3**: the interfaces, the notation rules, the synth behavior, the sound-effect table, the seven tune strings, and the WAV tool. Use the interfaces and the tune strings **verbatim**.
2. `docs/superpowers/specs/2026-09-08-retro-collection-design.md` section 3.8 for intent.
3. `src/game.h` (defines `music_track_id_t`), `src/sfx.h`, `tests/harness.h`, `Makefile`.

## The task

Do Task 3 steps 1 to 6 in order: write the failing tests (`tests/test_music.c`, `tests/test_synth.c`); implement `src/music.h`, `src/music.c` (parser, note table, `music_note_freq_q8`), `src/synth.h`, `src/synth.c` (four channels, envelopes, noise presets, sequencer, sound-effect voice, mixing, volume and enable flags); create `src/music_data.c` with the seven tunes copied exactly; create `tools/musicdump.c` and the `make music` target; verify; commit.

Implementation notes:
- `music_note_freq_q8`: table for octave 4 in Q8.8 (`C4 = 66977, C#4 = 70959, D4 = 75178, D#4 = 79649, E4 = 84385, F4 = 89403, F#4 = 94719, G4 = 100351, G#4 = 106318, A4 = 112640, A#4 = 119338, B4 = 126445`), shifted left for higher octaves and right for lower ones. `A4 = 69` must return exactly `440 << 8 = 112640`.
- Phase increment for a pulse or triangle: `inc = (uint32_t)(((uint64_t)freq_q8 << 8) / sample_rate)` in Q16 of a cycle per sample (`(freq / 256) * 65536 / rate`).
- The sequencer must produce exactly `steps * step_samples` samples per loop and then wrap; a `NULL` track renders silence from the music channels.
- Zero-crossing expectation in the test: a 440 Hz square rendered for one second at 22050 Hz crosses zero about 880 times.
- All integer math in `music.c` and `synth.c`; `tools/musicdump.c` may use `<stdio.h>` for the WAV writer.

## Standing rules

- `src/music.c`, `src/music.h`, `src/music_data.c`, `src/synth.c`, `src/synth.h` are pure modules: only `<stdint.h>`, `<stdbool.h>`, `<string.h>`, `<stddef.h>`; no floats, no stdio, no libdragon.
- Do not edit the plan or specs; do not touch `src/n64/app.c`, `src/app_state.*`, `src/menu.*`, or any game.
- The tune strings are the composer's work: if the parser rejects a bar, change only a rest length so that bar has 16 steps, and list every such change (tune, bar number, before/after) in your final report and the commit message. Do not otherwise alter notes.
- Every command must finish in under four minutes. Do not `git push`.

## Verification (all must pass before committing)

```
make test                                              # test_music and test_synth join the green binaries
grep -nE '\b(float|double)\b|#include <(math|stdio)\.h>|libdragon' src/music.c src/music.h src/music_data.c src/synth.c src/synth.h   # prints nothing
make music                                             # build/music/*.wav: 7 tunes + 11 effects
ls -la build/music/
```

Each tune WAV must be between 200 KB and 2 MB (two loops at 22050 Hz, 16-bit mono).

## When done

```
git add -A src tests tools Makefile
git commit -m "v2 Task 3: chiptune synth, notation sequencer, sound effects, seven tunes"
```

Then print a short report: `make test` summary lines, the `ls -la build/music/` listing, the step count per tune as parsed, and every tune-string fix you had to make (or "none").
