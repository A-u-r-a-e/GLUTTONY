#include "Helpers.hpp"
#include "input/Filter.hpp"

#include <doctest/doctest.h>

using namespace gluttony::input;
using namespace gluttony::input::test;

TEST_CASE("glob matching") {
    CHECK(globMatch("key/W", "key/W"));
    CHECK_FALSE(globMatch("key/W", "key/A"));
    CHECK(globMatch("key/*", "key/Space"));
    CHECK(globMatch("*", ""));
    CHECK(globMatch("dir/*/left", "dir/move/left"));
    CHECK_FALSE(globMatch("dir/*/left", "dir/move/right"));
    CHECK(globMatch("key/?", "key/A"));
    CHECK_FALSE(globMatch("key/?", "key/AB"));
    CHECK(globMatch("*a*b", "xxaxxb"));
}

TEST_CASE("selector ids and tags") {
    Selector s;
    s.ids = {"action/*"};
    s.allTags = {"movement"};
    s.noneTags = {"debug"};
    CHECK(s.matches("action/dash", {"movement", "action"}));
    CHECK_FALSE(s.matches("action/dash", {"action"}));
    CHECK_FALSE(s.matches("action/dash", {"movement", "debug"}));
    CHECK_FALSE(s.matches("key/W", {"movement"}));

    Selector any;
    any.anyTags = {"keyboard", "mouse"};
    CHECK(any.matches("x", {"mouse"}));
    CHECK_FALSE(any.matches("x", {"pad"}));
}

TEST_CASE("filter phases, magnitude and angle") {
    Filter f = Filter::id("aim", {Phase::Change});
    CHECK(f.matches(change("aim", {1, 0}, 0)));
    CHECK_FALSE(f.matches(pulse("aim", {1, 0}, 0)));

    f.minMagnitude = 2.f;
    CHECK_FALSE(f.matches(change("aim", {1, 0}, 0)));
    CHECK(f.matches(change("aim", {3, 0}, 0)));

    Filter dir = Filter::id("aim");
    dir.angle = 90.f; // screen down
    dir.arc = 90.f;
    CHECK(dir.matches(change("aim", {0.2f, 1.f}, 0)));
    CHECK_FALSE(dir.matches(change("aim", {1.f, 0.f}, 0)));
    CHECK_FALSE(dir.matches(change("aim", {0.f, 0.f}, 0)));
}
