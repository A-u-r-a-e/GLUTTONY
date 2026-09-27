# GLUTTONY

Top-down open-world action / power-scaling game where you grow by eating what you kill.
C++20 / SFML 3 / CMake (the Python files are an old Arcade prototype).

## Build & test

```sh
cmake -S . -B build -G Ninja && cmake --build build
./build/gluttony_tests        # must pass before committing
./build/gluttony              # input playground (needs a display; Xvfb + xdotool work headless)
```

## Layout

- `src/input/` input engine: pure C++, no SFML, time is fed in. See `docs/input-engine.md`.
- `src/app/` SFML playground app (event translation + debug view).
- `tests/` doctest unit tests for the engine.
- `data/input.json` bindings, processors, channels, mute groups (reloadable with F5).

## Start of every session

1. Read [`docs/handoff.md`](docs/handoff.md) — current state, next steps, and the full session log.
2. Read [`docs/design.md`](docs/design.md) — the living design doc.
3. For input work, read [`docs/input-engine.md`](docs/input-engine.md).

## End of every session

Update `docs/handoff.md`:
- Rewrite **Current state** and **Next up**.
- Add a new **Session log** entry at the top (what was discussed, decided, and left open).
- If design direction changed, update `docs/design.md` too.
- Commit and push so the next session has it.

## Working style

- **Keep everything flexible.** Don't impose fixed definitions, categories, or limits the
  user hasn't asked for. Prefer designs where things can be combined and extended.
- Don't split things into exclusive buckets (e.g. "keyboard does X, mouse does Y") —
  all inputs and systems should be usable together.
- Don't frame ideas as "risky" or push the user toward the conservative option.
  Offer possibilities; let the user decide.
- Names and terminology are placeholders until the user settles them.
- Content and tuning belong in data, not hard-coded in the engine.
