# Task 1: Skeleton, toolchain image, and a booting title ROM

You are implementing **Task 1** of an approved plan for a Nintendo 64 port of the iPod "Brick" game. You have no other context: read the files below first.

## Read first (in this order)

1. `docs/superpowers/plans/2026-09-08-ipod-brick-n64.md` — the implementation plan. Read the header, "Global Constraints", "File Structure", and all of **Task 1**. Use the code in Task 1 **verbatim**; it was verified against the libdragon trunk headers. Do not read ahead into other tasks' code except to understand interfaces.
2. `docs/superpowers/specs/2026-09-08-ipod-brick-n64-design.md` — the design spec, for background only.

## The task

Do every step of Task 1 in the plan, in order:

1. Create `Dockerfile` and `.dockerignore`.
2. Create `Makefile` (recipe lines need real tabs).
3. Create `src/brick.h` — the complete public API, verbatim from the plan.
4. Create `src/brick.c` — geometry helpers implemented, other functions stubbed, verbatim from the plan.
5. Create `tests/test_brick.c` — harness plus the five Task 1 tests, verbatim.
6. Run `make test`; it must print five `ok` lines and `0 failure(s)`.
7. Create `src/n64/app.c` — the minimal title-screen adapter, verbatim.
8. Create `scripts/ares-shot.sh` (verbatim) and `chmod +x` it.
9. Create the `README.md` stub.
10. Run `make image` (builds the Docker toolchain image; about one minute, downloads nothing besides a shallow libdragon clone), then `make rom` (produces `brick.z64`), then `scripts/ares-shot.sh brick.z64 build/shots/task1 4` (opens ares briefly and saves `build/shots/task1/shot-4s.png`).
11. Commit.

## Standing rules

- Environment: Apple Silicon Mac, Docker Desktop running, ares installed at `/Applications/ares.app`, clang from Xcode CLT. The Docker base image `ghcr.io/dragonminded/libdragon:latest-arm64` is already pulled.
- Never use mupen64plus; it cannot boot libdragon ROMs.
- Do not edit the plan or the spec. Do not change any signature in `src/brick.h`. Do not add features beyond Task 1.
- The core files `src/brick.h` and `src/brick.c` must not include `<libdragon.h>` or `<math.h>` and must not use `float` or `double`.
- If `make image`, `make rom`, or the screenshot script fails, fix the cause only if it is clearly in the files you created (typo, missing tab, wrong path). If the failure is in the toolchain, Docker, or ares, stop, leave the tree uncommitted, and report the exact error output.
- Do not `git push`.

## Verification (all must pass before committing)

```
make test                                   # 5 x ok, "0 failure(s)", exit 0
make image                                  # image ipod-brick-n64:dev built
make rom                                    # brick.z64 exists, roughly 200 KB
scripts/ares-shot.sh brick.z64 build/shots/task1 4   # build/shots/task1/shot-4s.png exists, > 20 KB
ls -la brick.z64 build/shots/task1/shot-4s.png
```

If you can view images, confirm the screenshot shows a light grey-beige screen with "BRICK" and "PRESS A" centered. If you cannot view images, say so in your report.

## When done

Stage exactly these paths and commit:

```
git add Dockerfile .dockerignore Makefile README.md src tests scripts
git commit -m "Task 1: skeleton, toolchain image, and booting title ROM"
```

Then print a short report: the `make test` output, the size of `brick.z64`, the screenshot path and size, and anything you deviated from or could not verify.
