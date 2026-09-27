#include "input/Signal.hpp"

#include <algorithm>
#include <numbers>

namespace gluttony::input {

float Vec2::angleDegrees() const {
    return std::atan2(y, x) * 180.f / std::numbers::pi_v<float>;
}

Vec2 Vec2::normalized() const {
    const float len = length();
    return len > 0.f ? Vec2{x / len, y / len} : Vec2{};
}

float angleDelta(float a, float b) {
    float d = std::fmod(a - b, 360.f);
    if (d <= -180.f) d += 360.f;
    if (d > 180.f) d -= 360.f;
    return d;
}

std::string_view phaseName(Phase phase) {
    switch (phase) {
    case Phase::Press: return "press";
    case Phase::Release: return "release";
    case Phase::Change: return "change";
    case Phase::Pulse: return "pulse";
    }
    return "?";
}

std::optional<Phase> phaseFromString(std::string_view text) {
    if (text == "press") return Phase::Press;
    if (text == "release") return Phase::Release;
    if (text == "change") return Phase::Change;
    if (text == "pulse") return Phase::Pulse;
    return std::nullopt;
}

bool Signal::hasTag(std::string_view tag) const {
    return std::find(tags.begin(), tags.end(), tag) != tags.end();
}

Signal Signal::derived(const Signal& cause, std::string id, Phase phase, Vec2 value,
                       std::vector<std::string> tags) {
    return Signal{std::move(id), phase, value, cause.time, std::move(tags), cause.depth + 1};
}

} // namespace gluttony::input
