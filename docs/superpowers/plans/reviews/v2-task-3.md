# Review: v2 Task 3 (commits e5e803d, d750c23)

Verdict: **pass** after one fix round; the user's listening review is pending and any tune changes will be string edits to `src/music_data.c`.

Checked:
- Parser, note table, sequencer, four-channel synth, sound-effect voice, and the WAV tool implemented per plan; `src/music_data.c` byte-identical to the plan's seven tunes; every tune parsed with no bar fixes (128 steps each). Six test binaries `0 failure(s)` (rerun by reviewer). Purity grep clean.
- Rendered WAVs measured (peak / RMS / silence): no silent stretches in any tune; effects peak at 8000 (16000 with a noise burst).
- Finding: Blocks, Brick, and Parachute hit full scale (32768) because the mixer scale of 160 put channel-gain sums of about 228 above the int16 range. Fix in d750c23: scale 110 and the single-channel test threshold lowered to 4000. Re-measured peaks now 18040 to 25080, leaving headroom for the 8000-level effect voice.
- Loudness balance is consistent across tunes (RMS 6800 to 8500) except Pong, which is sparse by design (4800).
