# Handoff

Read this at the start of every session. Update it at the end of every session.

- **Current state** and **Next up** are rewritten each session to reflect reality.
- **Session log** is append-only: add a new entry at the top, never delete old ones.
  Record what was discussed, what was decided (in the user's own framing), and what's still open.

---

## Current state

- Design brainstorm done for the high-level vision → see [`design.md`](design.md).
- Engine is switching from Python Arcade to **C++ / SFML**. No C++ code exists yet.
- The Python files (`main.py`, `window.py`, `datatypes/`, `content/`) are the old Arcade
  prototype. Not deleted — user hasn't asked for that.
- `README.md` still says Arcade (user's original notes, left as-is).

## Next up

- Plan and set up the C++ / SFML project (build system, layout).
- Build milestone 1: the input/combo engine playground (see `design.md`).
- Keep everything flexible — see working-style notes in `/CLAUDE.md`.

---

## Session log

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
