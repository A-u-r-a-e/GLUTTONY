#include "Helpers.hpp"
#include "input/Processors.hpp"

#include <doctest/doctest.h>

using namespace gluttony::input;
using namespace gluttony::input::test;

namespace {

std::vector<Signal> run(Processor& p, const Signal& s) {
    InputState state;
    std::vector<Signal> out;
    p.onSignal(s, state, out);
    return out;
}

} // namespace

TEST_CASE("bind: several sources hold one target") {
    Bind bind(Filter::id("key/Space"), "action/dash", {"action"});
    Bind bindMouse(Filter::id("mouse/Right"), "action/dash", {"action"});
    // One Bind can take many sources through its filter:
    Bind both(Filter{Selector{{"key/Space", "mouse/Right"}}}, "action/dash", {"action"});

    auto out = run(both, press("key/Space", 0.0));
    REQUIRE(out.size() == 1);
    CHECK(out[0].id == "action/dash");
    CHECK(out[0].phase == Phase::Press);
    CHECK(out[0].depth == 1);
    CHECK(out[0].hasTag("action"));

    CHECK(run(both, press("mouse/Right", 0.1)).empty());   // already held
    CHECK(run(both, release("key/Space", 0.2)).empty());   // still held by mouse
    out = run(both, release("mouse/Right", 0.3));
    REQUIRE(out.size() == 1);
    CHECK(out[0].phase == Phase::Release);

    CHECK(run(both, release("mouse/Right", 0.4)).empty()); // unmatched release is ignored
}

TEST_CASE("axis2d: press, change, release with normalized diagonals") {
    Axis2D axis({"move", {"movement"}, Filter::id("up"), Filter::id("down"), Filter::id("left"),
                 Filter::id("right"), true});
    auto out = run(axis, press("up", 0.0));
    REQUIRE(out.size() == 1);
    CHECK(out[0].phase == Phase::Press);
    CHECK(out[0].value.y == doctest::Approx(-1.f));

    out = run(axis, press("right", 0.1));
    REQUIRE(out.size() == 1);
    CHECK(out[0].phase == Phase::Change);
    CHECK(out[0].value.length() == doctest::Approx(1.f));

    run(axis, release("up", 0.2));
    out = run(axis, release("right", 0.3));
    REQUIRE(out.size() == 1);
    CHECK(out[0].phase == Phase::Release);
}

TEST_CASE("relative: from minus origin, only on change") {
    Relative rel({"aim", {}, "mouse/position", "game/player"});
    run(rel, change("game/player", {100, 100}, 0.0));
    auto out = run(rel, change("mouse/position", {150, 100}, 0.1));
    REQUIRE(out.size() == 1);
    CHECK(out[0].value == Vec2{50, 0});
    CHECK(run(rel, change("mouse/position", {150, 100}, 0.2)).empty());
}

TEST_CASE("sectors relative to a reference direction") {
    Sectors sectors({"dir/move", {}, "move", "aim", {"toward", "right", "away", "left"}, 0.f, 0.1f});

    run(sectors, change("aim", {0, 1}, 0.0)); // facing screen-down
    auto out = run(sectors, press("move", 0.1));
    // press() carries {1,0}: moving screen-right while facing down is to your left.
    REQUIRE(out.size() == 1);
    CHECK(out[0].id == "dir/move/left");
    CHECK(out[0].phase == Phase::Press);

    // Turning to face right makes the same movement "toward".
    out = run(sectors, change("aim", {1, 0}, 0.2));
    REQUIRE(out.size() == 2);
    CHECK(out[0].id == "dir/move/left");
    CHECK(out[0].phase == Phase::Release);
    CHECK(out[1].id == "dir/move/toward");

    out = run(sectors, release("move", 0.3));
    REQUIRE(out.size() == 1);
    CHECK(out[0].id == "dir/move/toward");
    CHECK(out[0].phase == Phase::Release);

    // Pulses map straight to sector pulses.
    out = run(sectors, pulse("move", {-5, 0}, 0.4));
    REQUIRE(out.size() == 1);
    CHECK(out[0].id == "dir/move/away");
    CHECK(out[0].phase == Phase::Pulse);
}

TEST_CASE("sectors without a reference are absolute; deadzone suppresses") {
    Sectors sectors({"dir/mouse", {}, "aim", std::nullopt, {"e", "s", "w", "n"}, 0.f, 10.f});
    CHECK(run(sectors, change("aim", {5, 0}, 0.0)).empty());
    auto out = run(sectors, change("aim", {0, -50}, 0.1));
    REQUIRE(out.size() == 1);
    CHECK(out[0].id == "dir/mouse/n");
}

TEST_CASE("flick needs speed and a measurable span") {
    const Flick::Spec spec{.to = "flick", .source = "mouse/position", .minSpeed = 1000.f,
                           .window = 0.1, .minSpan = 0.01, .cooldown = 0.1};
    Flick flick(spec);
    CHECK(run(flick, change("mouse/position", {0, 0}, 0.000)).empty());
    CHECK(run(flick, change("mouse/position", {50, 0}, 0.001)).empty()); // too close in time to judge
    auto out = run(flick, change("mouse/position", {60, 0}, 0.030));
    REQUIRE(out.size() == 1);
    CHECK(out[0].phase == Phase::Pulse);
    CHECK(out[0].value.x > 1000.f);

    CHECK(run(flick, change("mouse/position", {200, 0}, 0.060)).empty()); // cooldown

    Flick slow(spec);
    run(slow, change("mouse/position", {0, 0}, 0.0));
    CHECK(run(slow, change("mouse/position", {5, 0}, 0.05)).empty());
}

TEST_CASE("threshold turns an analog value into a button with hysteresis") {
    Threshold t({"stick/right", {}, Filter::id("pad0/axis/X"), 0.5f, 0.3f, false});
    CHECK(run(t, change("pad0/axis/X", {0.4f, 0}, 0.0)).empty());
    auto out = run(t, change("pad0/axis/X", {0.6f, 0}, 0.1));
    REQUIRE(out.size() == 1);
    CHECK(out[0].phase == Phase::Press);
    CHECK(run(t, change("pad0/axis/X", {0.4f, 0}, 0.2)).empty());
    out = run(t, change("pad0/axis/X", {0.1f, 0}, 0.3));
    REQUIRE(out.size() == 1);
    CHECK(out[0].phase == Phase::Release);
}
