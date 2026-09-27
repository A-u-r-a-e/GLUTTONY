#pragma once

#include "input/Signal.hpp"

#include <string>
#include <vector>

namespace gluttony::input::test {

inline Signal press(std::string id, Time t, std::vector<std::string> tags = {}) {
    return {std::move(id), Phase::Press, {1.f, 0.f}, t, std::move(tags)};
}

inline Signal release(std::string id, Time t, std::vector<std::string> tags = {}) {
    return {std::move(id), Phase::Release, {}, t, std::move(tags)};
}

inline Signal change(std::string id, Vec2 v, Time t, std::vector<std::string> tags = {}) {
    return {std::move(id), Phase::Change, v, t, std::move(tags)};
}

inline Signal pulse(std::string id, Vec2 v, Time t, std::vector<std::string> tags = {}) {
    return {std::move(id), Phase::Pulse, v, t, std::move(tags)};
}

} // namespace gluttony::input::test
