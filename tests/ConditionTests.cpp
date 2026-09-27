#include "Helpers.hpp"
#include "input/Conditions.hpp"

#include <doctest/doctest.h>

using namespace gluttony::input;
using namespace gluttony::input::test;

namespace {

const std::vector<Phase> kPress{Phase::Press};

/// Applies the signal to state first, as the engine does.
std::optional<Fire> send(Condition& c, InputState& state, const Signal& s) {
    state.apply(s);
    return c.onSignal(s, state);
}

} // namespace

TEST_CASE("on fires for matching signals only") {
    InputState state;
    auto c = cond::on(Filter::id("action/attack", kPress));
    CHECK(send(*c, state, press("action/attack", 1.0)));
    CHECK_FALSE(send(*c, state, release("action/attack", 1.1)));
    CHECK_FALSE(send(*c, state, press("action/dash", 1.2)));
}

TEST_CASE("hold fires after the duration, once per press") {
    InputState state;
    auto c = cond::hold(Filter::id("action/attack"), 0.5);
    CHECK_FALSE(send(*c, state, press("action/attack", 1.0)));
    CHECK_FALSE(c->onTick(1.4, state));
    auto fired = c->onTick(1.6, state);
    REQUIRE(fired);
    CHECK(fired->time == doctest::Approx(1.5));
    CHECK_FALSE(c->onTick(2.0, state));
}

TEST_CASE("hold does not fire when released early") {
    InputState state;
    auto c = cond::hold(Filter::id("action/attack"), 0.5);
    send(*c, state, press("action/attack", 1.0));
    CHECK_FALSE(send(*c, state, release("action/attack", 1.3)));
    CHECK_FALSE(c->onTick(2.0, state));
}

TEST_CASE("hold settles on release if no tick happened in between") {
    InputState state;
    auto c = cond::hold(Filter::id("action/attack"), 0.5);
    send(*c, state, press("action/attack", 1.0));
    CHECK(send(*c, state, release("action/attack", 1.7)));
}

TEST_CASE("sequence completes in order within the gap") {
    InputState state;
    std::vector<ConditionPtr> steps;
    steps.push_back(cond::on(Filter::id("action/dash", kPress)));
    steps.push_back(cond::on(Filter::id("action/attack", kPress)));
    auto c = cond::sequence(std::move(steps), 0.3);

    CHECK_FALSE(send(*c, state, press("action/dash", 1.0)));
    CHECK(c->progress() == doctest::Approx(0.5));
    CHECK_FALSE(send(*c, state, press("key/Unrelated", 1.1)));
    CHECK(send(*c, state, press("action/attack", 1.2)));
    CHECK(c->progress() == doctest::Approx(0.0));
}

TEST_CASE("sequence times out, then restarts from the same input") {
    InputState state;
    std::vector<ConditionPtr> steps;
    steps.push_back(cond::on(Filter::id("action/dash", kPress)));
    steps.push_back(cond::on(Filter::id("action/attack", kPress)));
    auto c = cond::sequence(std::move(steps), 0.3);

    send(*c, state, press("action/dash", 1.0));
    CHECK_FALSE(send(*c, state, press("action/attack", 1.5))); // too late
    send(*c, state, press("action/dash", 2.0));
    CHECK_FALSE(c->onTick(2.1, state));
    CHECK(c->progress() > 0.f);
    c->onTick(2.4, state);
    CHECK(c->progress() == doctest::Approx(0.0));
}

TEST_CASE("sequence per-step gap and breakOn") {
    InputState state;
    std::vector<cond::Step> steps;
    steps.push_back({cond::on(Filter::id("a", kPress)), std::nullopt});
    steps.push_back({cond::on(Filter::id("b", kPress)), 1.0});
    auto c = cond::sequence(std::move(steps), 0.1, Filter::id("stop"));

    send(*c, state, press("a", 0.0));
    CHECK(send(*c, state, press("b", 0.9))); // step gap overrides the 0.1 default

    send(*c, state, press("a", 2.0));
    send(*c, state, press("stop", 2.1));
    CHECK_FALSE(send(*c, state, press("b", 2.2)));
}

TEST_CASE("together fires when all parts land within the window, any order") {
    InputState state;
    std::vector<ConditionPtr> parts;
    parts.push_back(cond::on(Filter::id("action/guard", kPress)));
    parts.push_back(cond::on(Filter::id("action/venom", kPress)));
    auto c = cond::together(std::move(parts), 0.05);

    CHECK_FALSE(send(*c, state, press("action/venom", 1.00)));
    CHECK(send(*c, state, press("action/guard", 1.03)));

    CHECK_FALSE(send(*c, state, press("action/venom", 2.00)));
    CHECK_FALSE(send(*c, state, press("action/guard", 2.10)));
}

TEST_CASE("anyOf fires on either part") {
    InputState state;
    std::vector<ConditionPtr> parts;
    parts.push_back(cond::on(Filter::id("a", kPress)));
    parts.push_back(cond::on(Filter::id("b", kPress)));
    auto c = cond::anyOf(std::move(parts));
    CHECK(send(*c, state, press("b", 0.0)));
    CHECK(send(*c, state, press("a", 0.1)));
    CHECK_FALSE(send(*c, state, press("c", 0.2)));
}

TEST_CASE("gate checks what is currently active") {
    InputState state;
    auto whileGuard = cond::gate(cond::on(Filter::id("action/dash", kPress)), Filter::id("action/guard"));
    auto unlessGuard =
        cond::gate(cond::on(Filter::id("action/dash", kPress)), Filter::id("action/guard"), true);

    CHECK_FALSE(send(*whileGuard, state, press("action/dash", 0.0)));
    CHECK(unlessGuard->onSignal(press("action/dash", 0.0), state));

    state.apply(press("action/guard", 0.5));
    CHECK(send(*whileGuard, state, press("action/dash", 0.6)));
    CHECK_FALSE(unlessGuard->onSignal(press("action/dash", 0.6), state));
}

TEST_CASE("conditions nest: a sequence whose middle step is a chord") {
    InputState state;
    std::vector<ConditionPtr> chord;
    chord.push_back(cond::on(Filter::id("a", kPress)));
    chord.push_back(cond::on(Filter::id("b", kPress)));
    std::vector<ConditionPtr> steps;
    steps.push_back(cond::on(Filter::id("start", kPress)));
    steps.push_back(cond::together(std::move(chord), 0.05));
    steps.push_back(cond::hold(Filter::id("c"), 0.2));
    auto c = cond::sequence(std::move(steps), 0.5);

    send(*c, state, press("start", 0.0));
    send(*c, state, press("b", 0.10));
    send(*c, state, press("a", 0.12));
    send(*c, state, press("c", 0.3));
    CHECK(c->onTick(0.55, state));
}

TEST_CASE("clone gives independent state") {
    InputState state;
    std::vector<ConditionPtr> steps;
    steps.push_back(cond::on(Filter::id("a", kPress)));
    steps.push_back(cond::on(Filter::id("b", kPress)));
    auto original = cond::sequence(std::move(steps), 1.0);
    send(*original, state, press("a", 0.0));
    auto copy = original->clone();
    copy->reset();
    CHECK(original->progress() > 0.f);
    CHECK(copy->progress() == doctest::Approx(0.0));
}
