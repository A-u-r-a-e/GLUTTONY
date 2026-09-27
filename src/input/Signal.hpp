#pragma once

#include <cmath>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace gluttony::input {

/// Seconds on whatever clock the host feeds in. The engine never reads a clock itself.
using Time = double;

struct Vec2 {
    float x = 0.f;
    float y = 0.f;

    float length() const { return std::sqrt(x * x + y * y); }
    /// atan2(y, x) in degrees. With screen coordinates (y down), positive = clockwise.
    float angleDegrees() const;
    Vec2 normalized() const;

    friend Vec2 operator+(Vec2 a, Vec2 b) { return {a.x + b.x, a.y + b.y}; }
    friend Vec2 operator-(Vec2 a, Vec2 b) { return {a.x - b.x, a.y - b.y}; }
    friend Vec2 operator*(Vec2 a, float s) { return {a.x * s, a.y * s}; }
    friend bool operator==(Vec2 a, Vec2 b) = default;
};

/// Signed smallest difference a - b, in (-180, 180].
float angleDelta(float a, float b);

enum class Phase : std::uint8_t {
    Press,   ///< became active (button down, entered a sector, ...)
    Release, ///< became inactive
    Change,  ///< a continuous value moved (pointer position, axis, vector)
    Pulse,   ///< momentary, no duration (wheel tick, flick, channel fire)
};

std::string_view phaseName(Phase phase);
std::optional<Phase> phaseFromString(std::string_view text);

/// Everything that flows through the engine is a Signal: raw keys and mouse, game context
/// (e.g. the player's position), derived values (actions, directions) and channel fires.
struct Signal {
    std::string id;                ///< e.g. "key/W", "mouse/position", "action/dash", "channel/lunge"
    Phase phase = Phase::Pulse;
    Vec2 value{};                  ///< buttons: {1,0} down / {0,0} up; scalars use x
    Time time = 0.0;
    std::vector<std::string> tags; ///< classes used for filtering and muting
    int depth = 0;                 ///< 0 = fed from outside, +1 per derivation

    bool hasTag(std::string_view tag) const;

    /// A new signal caused by `cause`: same time, one level deeper.
    static Signal derived(const Signal& cause, std::string id, Phase phase, Vec2 value,
                          std::vector<std::string> tags);
};

} // namespace gluttony::input
