#pragma once

#include "input/Filter.hpp"
#include "input/InputState.hpp"
#include "input/Signal.hpp"

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace gluttony::input {

/// What a condition produces when it completes.
struct Fire {
    Time time = 0.0;
    Vec2 value{};
};

/// A stateful pattern watched over the signal stream. Conditions compose: a sequence of
/// chords of holds of ... anything. Add new kinds by subclassing.
class Condition {
public:
    virtual ~Condition() = default;

    virtual std::optional<Fire> onSignal(const Signal& signal, const InputState& state) = 0;
    virtual std::optional<Fire> onTick(Time /*now*/, const InputState& /*state*/) {
        return std::nullopt;
    }
    virtual void reset() {}
    /// 0 = idle .. 1 = complete. For debug displays.
    virtual float progress() const { return 0.f; }
    virtual std::string describe() const = 0;
    virtual std::unique_ptr<Condition> clone() const = 0;
};

using ConditionPtr = std::unique_ptr<Condition>;

namespace cond {

/// Fires on any signal matching the filter.
ConditionPtr on(Filter filter);

/// Fires once a matching signal has stayed active (pressed) for `duration`.
ConditionPtr hold(Filter filter, Time duration);

struct Step {
    ConditionPtr condition;
    std::optional<Time> gap; ///< overrides the sequence gap for reaching this step
};

/// Fires when the steps complete in order, each within `gap` of the previous one.
/// Unrelated signals in between are ignored unless they match `breakOn`, which restarts it.
ConditionPtr sequence(std::vector<Step> steps, Time gap, std::optional<Filter> breakOn = {});
ConditionPtr sequence(std::vector<ConditionPtr> steps, Time gap);

/// Fires when every part has fired within `window` of each other, in any order.
ConditionPtr together(std::vector<ConditionPtr> parts, Time window);

/// Fires when any part fires.
ConditionPtr anyOf(std::vector<ConditionPtr> parts);

/// Lets `inner` fire only while something matching `whileActive` is active
/// (or only while nothing is, when `invert`).
ConditionPtr gate(ConditionPtr inner, Filter whileActive, bool invert = false);

} // namespace cond

} // namespace gluttony::input
