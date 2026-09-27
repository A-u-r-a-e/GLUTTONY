#include "Helpers.hpp"
#include "input/Config.hpp"

#include <doctest/doctest.h>
#include <nlohmann/json.hpp>

#include <stdexcept>
#include <string>

using namespace gluttony::input;
using namespace gluttony::input::test;
using nlohmann::json;

TEST_CASE("the shipped data/input.json loads and works") {
    InputEngine engine;
    auto loaded = config::loadFile(std::string(GLUTTONY_DATA_DIR) + "/input.json", engine);
    CHECK_FALSE(engine.channels().empty());
    CHECK_FALSE(loaded.muteGroups.empty());

    int lunges = 0;
    engine.listen(Filter::id("channel/wolf_lunge"), [&](const Signal&) { ++lunges; });
    engine.feed(press("key/Space", 1.0, {"raw", "keyboard", "button"}));
    engine.feed(press("mouse/Left", 1.1, {"raw", "mouse", "button"}));
    CHECK(lunges == 1);
}

TEST_CASE("condition shorthand and modifiers") {
    InputEngine engine;
    config::apply(json::parse(R"({
        "channels": [
            { "name": "release_attack", "when": { "on": "action/attack", "phase": "release" } },
            { "name": "dash_unless_guard", "when": { "on": "action/dash", "unless": "action/guard" } }
        ]
    })"),
                  engine);

    engine.feed(press("action/attack", 0.0));
    CHECK(engine.channel("release_attack")->fireCount() == 0);
    engine.feed(release("action/attack", 0.1));
    CHECK(engine.channel("release_attack")->fireCount() == 1);

    engine.feed(press("action/guard", 0.2));
    engine.feed(press("action/dash", 0.3));
    CHECK(engine.channel("dash_unless_guard")->fireCount() == 0);
    engine.feed(release("action/guard", 0.4));
    engine.feed(press("action/dash", 0.5));
    CHECK(engine.channel("dash_unless_guard")->fireCount() == 1);
}

TEST_CASE("errors point at the offending entry") {
    InputEngine engine;
    const json bad = json::parse(R"({ "channels": [ { "name": "x", "when": { "on": "a", "phase": "bogus" } } ] })");
    try {
        config::apply(bad, engine);
        FAIL("expected an error");
    } catch (const std::runtime_error& e) {
        CHECK(std::string(e.what()).find("channels[0].when.phase") != std::string::npos);
    }

    CHECK_THROWS_AS(config::apply(json::parse(R"({ "processors": [ { "type": "nope", "to": "x" } ] })"), engine),
                    std::runtime_error);
}
