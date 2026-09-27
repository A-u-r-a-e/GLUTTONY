#pragma once

#include "input/Filter.hpp"
#include "input/InputState.hpp"
#include "input/Signal.hpp"

#include <deque>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace gluttony::input {

/// Turns signals into more signals (actions, vectors, directions, ...). Outputs are fed back
/// into the engine, so processors chain and everything downstream can watch them.
/// Processors keep their own state from the signals they've seen, so muting stays consistent.
class Processor {
public:
    virtual ~Processor() = default;
    virtual void onSignal(const Signal& in, const InputState& state, std::vector<Signal>& out) = 0;
    virtual void onTick(Time /*now*/, const InputState& /*state*/, std::vector<Signal>& /*out*/) {}
    virtual void reset() {}
    virtual std::string describe() const = 0;
};

/// Re-emits matching signals under a new id. Any number of sources can feed one target;
/// the target stays pressed while any source is pressed.
class Bind final : public Processor {
public:
    Bind(Filter from, std::string to, std::vector<std::string> tags);
    void onSignal(const Signal& in, const InputState& state, std::vector<Signal>& out) override;
    void reset() override { active_.clear(); }
    std::string describe() const override;

private:
    Filter from_;
    std::string to_;
    std::vector<std::string> tags_;
    std::set<std::string, std::less<>> active_;
};

/// Combines four sets of buttons into a vector (screen convention: up = -y).
/// Press when it leaves zero, Change while moving, Release when it returns to zero.
class Axis2D final : public Processor {
public:
    struct Spec {
        std::string to;
        std::vector<std::string> tags;
        Filter up, down, left, right;
        bool normalize = true;
    };
    explicit Axis2D(Spec spec);
    void onSignal(const Signal& in, const InputState& state, std::vector<Signal>& out) override;
    void reset() override;
    std::string describe() const override;

private:
    Spec spec_;
    std::set<std::string, std::less<>> held_[4];
    Vec2 current_{};
};

/// Emits `from - origin` (e.g. mouse position relative to the player).
/// Without an origin it just mirrors `from`.
class Relative final : public Processor {
public:
    struct Spec {
        std::string to;
        std::vector<std::string> tags;
        std::string from;
        std::optional<std::string> origin;
    };
    explicit Relative(Spec spec);
    void onSignal(const Signal& in, const InputState& state, std::vector<Signal>& out) override;
    void reset() override;
    std::string describe() const override;

private:
    Spec spec_;
    Vec2 from_{}, origin_{};
    std::optional<Vec2> last_;
};

/// Splits the direction of a vector signal into named sectors, optionally measured
/// relative to another vector (e.g. movement relative to aim). Emits "<to>/<name>":
/// continuous sources Press/Release sectors as they're entered/left, pulses emit pulses.
class Sectors final : public Processor {
public:
    struct Spec {
        std::string to;
        std::vector<std::string> tags;
        std::string source;
        std::optional<std::string> reference;
        std::vector<std::string> names; ///< names[0] is centred on the reference direction
        float offsetDegrees = 0.f;
        float deadzone = 0.f;
    };
    explicit Sectors(Spec spec);
    void onSignal(const Signal& in, const InputState& state, std::vector<Signal>& out) override;
    void reset() override;
    std::string describe() const override;

    /// Sector index for a vector, or nullopt inside the deadzone.
    std::optional<std::size_t> sectorOf(Vec2 v) const;

private:
    void moveTo(std::optional<std::size_t> sector, const Signal& cause, std::vector<Signal>& out);
    std::string idFor(std::size_t sector) const { return spec_.to + "/" + spec_.names[sector]; }

    Spec spec_;
    Vec2 reference_{1.f, 0.f};
    Vec2 source_{};
    bool sourceActive_ = false;
    std::optional<std::size_t> current_;
};

/// Detects fast motion of a position signal. Pulses with the velocity (units/second).
class Flick final : public Processor {
public:
    struct Spec {
        std::string to;
        std::vector<std::string> tags;
        std::string source;
        float minSpeed = 2000.f;
        Time window = 0.08;
        Time minSpan = 0.016; ///< ignore bursts of samples too close together to measure
        Time cooldown = 0.15;
    };
    explicit Flick(Spec spec);
    void onSignal(const Signal& in, const InputState& state, std::vector<Signal>& out) override;
    void reset() override;
    std::string describe() const override;

private:
    struct Sample {
        Time time;
        Vec2 pos;
    };
    Spec spec_;
    std::deque<Sample> history_;
    std::optional<Time> lastFlick_;
};

/// Turns a scalar (x) or vector magnitude into a button: Press above `pressAt`,
/// Release below `releaseAt`. Useful for analog sticks, triggers, wheel speed, ...
class Threshold final : public Processor {
public:
    struct Spec {
        std::string to;
        std::vector<std::string> tags;
        Filter from;
        float pressAt = 0.5f;
        float releaseAt = 0.4f;
        bool useMagnitude = false; ///< compare |value| instead of value.x
    };
    explicit Threshold(Spec spec);
    void onSignal(const Signal& in, const InputState& state, std::vector<Signal>& out) override;
    void reset() override { active_ = false; }
    std::string describe() const override;

private:
    Spec spec_;
    bool active_ = false;
};

} // namespace gluttony::input
