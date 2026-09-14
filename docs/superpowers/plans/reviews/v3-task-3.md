# Review: v3 Task 3 (commit cdd93ad)

Verdict: **pass** on host evidence; emulator capture deferred.

Checked:
- `flap.c`: gravity 77, flap −1280, terminal 1536, ceiling clamp, pipe pairs 24 px wide spawned 90 px apart with centers in 70..180 and gaps from `flap_gap_for_score` (64 shrinking to 48), once-per-pipe scoring with `SFX_POINT`, `SFX_HIT` per flap, collisions with pipes and the ground ending the run with `SFX_EXPLODE`, overlays. Registered eighth with `MUSIC_FLAP`.
- Fifteen test binaries `0 failure(s)` (rerun by reviewer); autoplay scores 34 and ends at tick 1,639. Purity grep clean; no trademarked titles in `src` or `tests`. Autoplay ROM builds with no warnings.
- Host frame dump viewed: bird with eye threading green pipe pairs above the dark ground strip.
- ares window capture failed for both Grok and the reviewer with "could not create image from window": the macOS Screen Recording permission for the terminal appears to have lapsed. Emulator captures for Flap will be taken with Runner's once the permission is restored, before Task 5's evidence.

Accepted deviations: autoplay flaps when 22 px below the next gap center (a flap rises about 45 px, so the spec's 4 px threshold overshoots) with an emergency flap near the lower pipe; the bird starts at y 90 so the fixed-seed course is passable.
