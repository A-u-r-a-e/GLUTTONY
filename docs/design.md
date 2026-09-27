# GLUTTONY — Living Design Doc

This doc captures the vision and the direction we're heading. **Nothing here is fixed.**
Every mechanic, name, number, and category is open to change, extension, or combination.
When in doubt, pick the more flexible option and let playtesting decide.

---

## Vision

A top-down, open-world 2D action / power-scaling game with smooth (non-pixel) animation.
You grow by eating what you kill.

## Core pillars

### One persistent world
- A single long save; power keeps growing and what you've eaten stays with you.
- Open world, with some Metroidvania-style gating (what you've eaten can open up the world).

### Top-down, aim anywhere
- The player faces/aims freely. Abilities can take any shape (cones, slices, arcs, areas…).

### Gluttony
- On death, enemies have a chance to drop a corpse.
- Corpses can't be picked up — you eat them where they fall.
  (Other corpse uses, e.g. corpse explosion, are still on the table.)
- Eating starts with a **comprehension** phase (progress toward absorbing what that mob offers),
  then continued eating builds **Attenuation** (working name).
- What Attenuation does is **specific to each ability** — could be power, timing leniency,
  new behaviour, anything.
- Terminology is open: "skill" isn't quite right — ability / power / something else, TBD.

### The tree
- One big, complex, **authored** tree of everything you can gain.
- Mobs can unlock **anything** in it: branches, types, abilities, modifiers, enhancements,
  traits, traversal, or kinds of things we haven't thought of yet. Different mobs unlock
  different kinds of things.
- The engine must not hard-code what kinds of nodes exist. Content lives in data.

### Casting is a skill
- Abilities are activated by performing **combos**, like a fighting game / martial art —
  there should be real player skill to it.
- Some things are easy (a single input, or a toggle that just stays on); others are demanding.
- **Every input is part of one vocabulary.** Movement, attacks, runes/types, mouse buttons,
  mouse position, mouse motion — all of it can be used together, at the same time, in any
  combination. Like martial arts: you don't delegate movement and attack to different parts
  of the body.
- Keybinds are set by the player.
- Combos can involve sequences, simultaneous inputs, holds, releases, timing, directions —
  whatever makes a move feel right. Directions can come from movement, from where the mouse
  is, from how the mouse moves, or several at once.
- Longer combos are designed as **extensions** or **alternative routes** of shorter ones,
  so reaching any point in a combo can fire immediately with no waiting (good design, not
  engine hacks, solves ambiguity).
- If something along a combo path isn't unlocked yet, the inputs just do what they'd
  normally do.

## Tech

- **C++ with SFML** (moving away from Python Arcade; the Python files in the repo are the
  old prototype).
- Everything tunable (timings, bindings, directions, the tree, mobs) should be data-driven
  so the design can keep evolving without engine rewrites.

## First milestone — the input engine

Build the combo/input system on its own before enemies, world, or art:
- Read all input sources together and let combos reference any of them.
- The combo tree, loaded from data.
- A live debug view: see the tree, where you are in it, timing, and what's being recognised.

The goal is a playground to *feel out* the casting system, not to lock it down.

## Open / undecided (intentionally)

Names (Attenuation, ability, types…), what node kinds exist, tree content, how comprehension
is measured, world structure, combat details, art pipeline, how many directions and what
they're relative to, timing numbers — all open.
