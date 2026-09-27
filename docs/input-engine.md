# Input Engine

`src/input/` — plain C++20, no SFML. It never reads a clock; the host feeds timestamps.
That makes it deterministic and unit-testable (`tests/`).

## The model: everything is a Signal

A `Signal` has an `id`, a `phase` (press / release / change / pulse), a `value` (Vec2),
a `time`, `tags` (classes), and a `depth` (how many derivations deep it is).

Raw keys, mouse buttons, mouse position, wheel, gamepad, game context (player position),
actions, directions, flicks and channel fires are **all signals**. So anything can watch,
combine, or mute anything else. There are no fixed input categories: tags are free-form.

```
feed(signal) ─► state ─► mutes ─► listeners ─► processors ─► channels
                                                   │             │
                                                   └── derived signals fed back in ──┘
update(now)  ─► expire timed mutes, tick processors + channels (holds, timeouts)
```

## Pieces

| Piece | What it does | File |
|---|---|---|
| `InputEngine` | the hub: feed, update, add/remove processors/channels, mute, listen, introspect | `InputEngine.*` |
| `InputState` | what every id is doing right now (active, value, held-for). Tracks everything, muted or not | `InputState.*` |
| `Filter` / `Selector` | match signals (id globs, tags all/any/none, phases, magnitude, angle) / match channels | `Filter.*` |
| `Processor` | turns signals into more signals | `Processors.*` |
| `Condition` | composable, stateful pattern over the stream | `Conditions.*` |
| `Channel` | watches one condition; fires a `channel/<name>` pulse (+ optional callback) | `Channel.*` |
| `MuteTable` | stacked, independently removable, optionally timed mutes | `Mute.*` |
| `config` | builds all of the above from JSON | `Config.*` |

### Processors (built in; add more by subclassing `Processor`)
- **bind**: re-emit matching signals under a new id (keybinds → actions). Many-to-one, one-to-many.
- **axis2d**: four button sets → a vector.
- **relative**: `from - origin` (e.g. mouse position relative to the player → aim).
- **sectors**: a vector's direction, optionally relative to another vector, split into named sectors.
  Continuous sources press/release sectors; pulses emit pulses.
- **flick**: fast motion of a position → velocity pulse.
- **threshold**: analog → button with hysteresis.

### Conditions (compose freely; add more by subclassing `Condition`)
- **on**: a single matching signal.
- **hold**: something stays active for a duration.
- **seq**: steps in order, each within a gap (per-step gaps allowed); optional `breakOn`.
- **together**: all parts within a window, any order (chords).
- **any**: first part to fire.
- **while / unless**: gate any condition on what's currently active.

### Muting
Rules stack and each one is removed on its own. Each rule can be timed (`until`).

| `signals` | `channels` | effect |
|---|---|---|
| set | – | matching signals dropped from the whole pipeline |
| – | set | matching channels suspended |
| set | set | those channels stop seeing those signals |
| – | – | everything muted |

Processors can also be switched off individually (`setProcessorEnabled`), and channels
disabled (`Channel::setEnabled`).

## Config

`data/input.json` is the living example and documents the format inline (comments allowed).
Errors throw with a path to the bad entry, e.g. `channels[3].when.seq[1].phase`.

## Playground app

`src/app/` — SFML window that feeds real input through the engine and shows the signal log,
channels (progress + flash on fire), active signals, and mute toggles.

- **F5** reload `data/input.json`
- **F9** show/hide continuous signals (mouse position etc.) in the log
- Mute toggle keys come from `muteGroups` in the config (F1–F4, F6 by default)
