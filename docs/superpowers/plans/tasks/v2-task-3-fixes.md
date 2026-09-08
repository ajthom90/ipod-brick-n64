# v2 Task 3 fixes: mixer headroom

Review of your Task 3 commit: parser, sequencer, synth, effects, tunes, and the WAV tool are all correct and the tunes parse exactly as written. One measurement problem: `blocks.wav`, `brick.wav`, and `parachute.wav` reach the full-scale limit (peak 32768), so the mix clips whenever all four channels peak together. Their channel gains sum to about 220 to 228, and `MIX_SCALE = 160` puts that sum at about 36000, above the int16 range.

Do exactly this:

1. In `src/synth.c`, change `MIX_SCALE` from 160 to **110** (a single channel at gain 100 then peaks at 11000; a four-channel sum of 228 peaks at about 25000, leaving room for the 8000 sound-effect voice).
2. In `tests/test_synth.c`, the "peak exceeds 8000" expectation for the single A4 pulse channel at the default gain must become **"peak exceeds 4000"** (the channel now peaks at 6600). Do not change any other test.
3. Run `make test` and `make music`.
4. Check the peaks with this command and confirm no tune reaches 32767 or -32768:
   ```
   python3 -c "import wave,array,os; d='build/music'; [print(f, max(abs(x) for x in (lambda a:(a.frombytes(wave.open(d+'/'+f).readframes(10**8)),a)[1])(array.array('h')))) for f in sorted(os.listdir(d)) if f.endswith('.wav') and not f.startswith('sfx')]"
   ```
5. Commit:
   ```
   git add src/synth.c tests/test_synth.c
   git commit -m "v2 Task 3: lower mixer scale so full tunes do not clip"
   ```
   Do not stage anything else. Do not push. Keep every command under four minutes.

Then print the peak values for the seven tunes.
