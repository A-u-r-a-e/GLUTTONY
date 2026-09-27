# Handoff

Read this at the start of every session. Update it at the end of every session.

- **Current state** and **Next up** are rewritten each session to reflect reality.
- **Session log** is append-only: add a new entry at the top, never delete old ones.
  Record what was discussed, what was decided (in the user's own framing), and what's still open.

---

## Current state

- Design vision → [`design.md`](design.md). Input engine → [`input-engine.md`](input-engine.md).
- **C++20 / SFML 3.1 / CMake project is set up** (deps fetched automatically). Builds clean
  with no warnings; 36 unit tests pass.
- **Input engine core exists** (`src/input/`): everything is a Signal; processors derive new
  signals (bind, axis2d, relative, sectors, flick, threshold); channels watch composable
  conditions (on, hold, seq, together, any, while/unless); stacked mutes by signal class,
  channel class, or both (scoped), optionally timed; processors/channels can be switched off.
  All configurable from `data/input.json`.
- **Playground app** (`src/app/`) feeds real keyboard/mouse/gamepad input and shows the signal
  log, channel progress/fires, active signals and mute toggles. F5 reloads the config.
  Verified working under Xvfb with scripted input (lunge, follow-up, charge, chord, flick,
  guarded dash, mutes).
- `data/input.json` content (actions, runes, channels) is **placeholder** to play with.
- The Python files (`main.py`, `window.py`, `datatypes/`, `content/`) are the old Arcade
  prototype — not deleted, user hasn't asked for that. README intro still says Arcade.

## Next up

Ideas, not commitments — user decides:
- User plays with the playground and tunes `data/input.json`; see what feels right.
- The combo **tree** on top of channels (nodes, locked/unlocked, extensions/alternate routes,
  ability-specific data) — the design's "authored master tree".
- More processors/conditions as needed (e.g. repeat/count, "released within", axis-to-sector
  on gamepad sticks, input recording/replay for tuning).
- Rebinding UI / saving player keybinds separately from the authored channels.

---

## Session log

### 2026-09-27 (2) — C++ / SFML setup + input engine

- Set up CMake project: SFML 3.1 (bundled FreeType/HarfBuzz via `SFML_USE_SYSTEM_DEPS OFF`),
  nlohmann/json 3.12, doctest 2.5.3, all via FetchContent.
- Built the input engine per the user's ask: "channels to watch certain types of configurable
  event conditions as well as ways to mute classes of inputs". Kept it flexible: free-form
  tags instead of fixed categories, everything (raw, derived, channel fires) is one signal
  stream, so any input can combine with any other. Mouse can give direction by where it is
  (`dir/mouse`, `dir/aim_vs_move`) and by motion (`flick` → `dir/flick`); keyboard movement
  gives `dir/move` relative to aim. All at once.
- Mutes: global by signal class, channel class, or scoped (these signals hidden from those
  channels); stacked, independently removable, optional expiry.
- Playground app + debug view; tests; docs (`docs/input-engine.md`), README build section.
- Notes: `Sectors` with a reference keeps the last reference direction when the reference
  goes to zero (e.g. `dir/aim_vs_move` stays relative to the last movement direction).

### 2026-09-27 — Initial brainstorm

Talked through the core design one question at a time. Decisions (all still flexible):

1. **Loop:** one persistent world where power keeps growing, plus some Metroidvania-style
   gating from what you've eaten.
2. **Camera:** top-down, aim anywhere.
3. **Using abilities:** neither "infinite equipped rotation" nor "skill slots" from the
   README. Instead: abilities are activated by performing combos, fighting-game / martial-arts
   style, so there's real skill. Easy stuff can be one input or a toggle. Keybinds are
   player-set.
4. **Input vocabulary:** movement, attacks, runes/types — all one vocabulary, freely mixed.
   "You don't delegate movement and attack to different parts of your body."
5. **Combo ambiguity:** solved by design — longer combos are extensions or alternative
   routes of shorter ones.
6. **Tree:** one authored master tree (same for every player).
7. **Eating repeats:** comprehension first, then "Attenuation" (working name). What it does
   is ability-specific. "Skill" isn't the right word — ability/power, TBD.
8. **Tree contents:** anything — some mobs unlock branches, some types, some modifiers or
   enhancements, etc. Not decided and shouldn't be forced.
9. **First milestone:** the input engine only.
10. **Engine:** C++ / SFML, not Python Arcade.
11. **Combo steps:** sequences, chords, holds, releases, directions. Direction can come from
    movement *and* mouse (position, motion) — all usable together.

**Feedback from user this session:** Claude over-defined and over-separated things
(e.g. splitting input sources into exclusive categories, framing choices as "risky").
User wants everything kept flexible and combinable. See `/CLAUDE.md`.

Also set up this handoff system (`CLAUDE.md` + `docs/handoff.md` + `docs/design.md`).
