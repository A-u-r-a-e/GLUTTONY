#include "Helpers.hpp"
#include "input/InputEngine.hpp"

#include <doctest/doctest.h>

using namespace gluttony::input;
using namespace gluttony::input::test;

namespace {

const std::vector<Phase> kPress{Phase::Press};

/// Keyboard -> actions, plus a dash > attack channel.
void setup(InputEngine& engine) {
    engine.addProcessor("dash", std::make_unique<Bind>(Filter::id("key/Space"), "action/dash",
                                                       std::vector<std::string>{"action", "movement"}));
    engine.addProcessor("attack", std::make_unique<Bind>(Filter::id("mouse/Left"), "action/attack",
                                                         std::vector<std::string>{"action", "combat"}));
    std::vector<ConditionPtr> steps;
    steps.push_back(cond::on(Filter::id("action/dash", kPress)));
    steps.push_back(cond::on(Filter::id("action/attack", kPress)));
    ChannelSpec spec;
    spec.name = "lunge";
    spec.tags = {"combat"};
    spec.when = cond::sequence(std::move(steps), 0.3);
    engine.addChannel(std::move(spec));
}

void lunge(InputEngine& engine, Time t) {
    engine.feed(press("key/Space", t, {"keyboard"}));
    engine.feed(release("key/Space", t + 0.05, {"keyboard"}));
    engine.feed(press("mouse/Left", t + 0.1, {"mouse"}));
    engine.feed(release("mouse/Left", t + 0.15, {"mouse"}));
}

} // namespace

TEST_CASE("raw input flows through bindings into a channel and back out as a signal") {
    InputEngine engine;
    setup(engine);
    int heard = 0;
    engine.listen(Filter::id("channel/lunge"), [&](const Signal& s) {
        ++heard;
        CHECK(s.hasTag("channel"));
        CHECK(s.hasTag("combat"));
    });

    lunge(engine, 1.0);
    CHECK(heard == 1);
    CHECK(engine.channel("lunge")->fireCount() == 1);
    CHECK(engine.state().find("action/dash") != nullptr);
}

TEST_CASE("muting a class of raw input drops it from the pipeline but not from state") {
    InputEngine engine;
    setup(engine);
    const MuteId id = engine.mute({"no keyboard", Filter::tag("keyboard"), std::nullopt, std::nullopt});

    lunge(engine, 1.0);
    CHECK(engine.channel("lunge")->fireCount() == 0);
    CHECK(engine.state().find("key/Space") != nullptr); // still known
    CHECK(engine.state().find("action/dash") == nullptr); // but nothing reacted

    engine.unmute(id);
    lunge(engine, 2.0);
    CHECK(engine.channel("lunge")->fireCount() == 1);
}

TEST_CASE("muting derived classes (e.g. all movement actions)") {
    InputEngine engine;
    setup(engine);
    engine.mute({"no movement", Filter::tag("movement"), std::nullopt, std::nullopt});
    lunge(engine, 1.0);
    CHECK(engine.channel("lunge")->fireCount() == 0);
}

TEST_CASE("muting channels by tag suspends them") {
    InputEngine engine;
    setup(engine);
    Selector combat;
    combat.allTags = {"combat"};
    const MuteId id = engine.mute({"no combat", std::nullopt, combat, std::nullopt});
    CHECK(engine.isChannelMuted(*engine.channel("lunge")));
    lunge(engine, 1.0);
    CHECK(engine.channel("lunge")->fireCount() == 0);
    engine.unmute(id);
    lunge(engine, 2.0);
    CHECK(engine.channel("lunge")->fireCount() == 1);
}

TEST_CASE("scoped mute hides signals from some channels only") {
    InputEngine engine;
    setup(engine);
    ChannelSpec other;
    other.name = "any_attack";
    other.when = cond::on(Filter::id("action/attack", kPress));
    engine.addChannel(std::move(other));

    Selector lungeOnly;
    lungeOnly.ids = {"lunge"};
    engine.mute({"lunge ignores mouse", Filter::tag("mouse"), lungeOnly, std::nullopt});
    lunge(engine, 1.0);
    // The raw mouse press is hidden from "lunge", but the derived action isn't tagged mouse.
    CHECK(engine.channel("lunge")->fireCount() == 1);
    CHECK(engine.channel("any_attack")->fireCount() == 1);

    engine.mute({"lunge ignores combat", Filter::tag("combat"), lungeOnly, std::nullopt});
    lunge(engine, 2.0);
    CHECK(engine.channel("lunge")->fireCount() == 1);
    CHECK(engine.channel("any_attack")->fireCount() == 2);
}

TEST_CASE("timed mutes expire") {
    InputEngine engine;
    setup(engine);
    engine.mute({"brief", Filter::tag("keyboard"), std::nullopt, 1.5});
    lunge(engine, 1.0);
    CHECK(engine.channel("lunge")->fireCount() == 0);
    engine.update(1.6);
    CHECK(engine.mutes().rules().empty());
    lunge(engine, 2.0);
    CHECK(engine.channel("lunge")->fireCount() == 1);
}

TEST_CASE("channels can watch other channels") {
    InputEngine engine;
    setup(engine);
    std::vector<ConditionPtr> steps;
    steps.push_back(cond::on(Filter::id("channel/lunge")));
    steps.push_back(cond::on(Filter::id("channel/lunge")));
    ChannelSpec spec;
    spec.name = "double_lunge";
    spec.when = cond::sequence(std::move(steps), 1.0);
    engine.addChannel(std::move(spec));

    lunge(engine, 1.0);
    lunge(engine, 1.5);
    CHECK(engine.channel("double_lunge")->fireCount() == 1);
}

TEST_CASE("a channel feeding itself terminates via the depth guard") {
    InputEngine engine({.maxDepth = 5, .logSize = 64});
    ChannelSpec spec;
    spec.name = "echo";
    spec.when = cond::anyOf([] {
        std::vector<ConditionPtr> parts;
        parts.push_back(cond::on(Filter::id("key/E", {Phase::Press})));
        parts.push_back(cond::on(Filter::id("channel/echo")));
        return parts;
    }());
    engine.addChannel(std::move(spec));
    engine.feed(press("key/E", 0.0));
    // Fires at depths 0..5; the depth-6 echo is dropped.
    CHECK(engine.channel("echo")->fireCount() == 6);
}

TEST_CASE("update drives time-based conditions") {
    InputEngine engine;
    ChannelSpec spec;
    spec.name = "charge";
    spec.when = cond::hold(Filter::id("key/F"), 0.5);
    engine.addChannel(std::move(spec));
    engine.feed(press("key/F", 1.0));
    engine.update(1.2);
    CHECK(engine.channel("charge")->fireCount() == 0);
    engine.update(1.6);
    CHECK(engine.channel("charge")->fireCount() == 1);
}

TEST_CASE("cooldown and disabling") {
    InputEngine engine;
    ChannelSpec spec;
    spec.name = "tap";
    spec.when = cond::on(Filter::id("key/T", {Phase::Press}));
    spec.cooldown = 0.5;
    Channel& tap = engine.addChannel(std::move(spec));
    engine.feed(press("key/T", 0.0));
    engine.feed(press("key/T", 0.2));
    CHECK(tap.fireCount() == 1);
    tap.setEnabled(false);
    engine.feed(press("key/T", 1.0));
    CHECK(tap.fireCount() == 1);
}

TEST_CASE("processors can be switched off") {
    InputEngine engine;
    setup(engine);
    engine.setProcessorEnabled("dash", false);
    lunge(engine, 1.0);
    CHECK(engine.channel("lunge")->fireCount() == 0);
}
