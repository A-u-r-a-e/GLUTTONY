# GLUTTONY
i watched too much anime and now i'm going to make a gluttony game IN ARCADE


## Details
- open world 2d action/power scaling game
- art style: smooth animation style (not pixel art)

## Unique Premise
- upon enemy death, has a chance to drop corpse
- corpses can be consumed to gain skills - only skills, and all are related to mob killed
- corpses can not be picked up -> you have to consume them at the death spot (but maybe they can be used for like corpse explosion lol)

## Unique Skill Features

### General
- skills are in passive and active categories
- maybe skills can be merged - hard coded merges -> item

### Path One - infinite equipped skills
- as there are not infinite keybinds, you can bind skills to activate in order one after the other with the following options
1. skills are in fixed rotation and must all be activated
2. the rotation reset after some amount of inactivity
3. the rotation disregards order, is a priority queue of whichever cooldown has most recently been updated, choosing to add to front or back of stack

### Path Two - skill slots
- probably a lot easier to optimize
- also a lot lamer
- skill loadouts that can be hotkeyed
- skill merging becomes a must if we go here

## Building (C++ / SFML)

Needs CMake 3.28+ and a C++20 compiler. SFML 3, nlohmann/json and doctest are downloaded
automatically on first configure.

```sh
cmake -S . -B build
cmake --build build
./build/gluttony          # input playground (Windows: build\Debug\gluttony.exe)
./build/gluttony_tests    # unit tests
```

On Linux, SFML needs the X11/OpenGL/udev dev packages, e.g.
`sudo apt install libx11-dev libxrandr-dev libxcursor-dev libxi-dev libudev-dev libgl1-mesa-dev`.

Build just the engine and tests (no SFML): `cmake -S . -B build -DGLUTTONY_BUILD_APP=OFF`.

Design notes live in [`docs/`](docs/).
