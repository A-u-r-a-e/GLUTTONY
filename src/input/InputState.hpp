#pragma once

#include "input/Filter.hpp"
#include "input/Signal.hpp"

#include <map>
#include <optional>
#include <string>
#include <string_view>

namespace gluttony::input {

struct SignalRecord {
    Signal last;           ///< most recent signal with this id
    bool active = false;   ///< between a Press and its Release
    Time activeSince = 0.0;
};

/// What every signal id is currently doing. Tracks everything that happens, muted or not:
/// muting stops the engine reacting, it doesn't pretend the input didn't happen.
class InputState {
public:
    void apply(const Signal& signal);
    void clear() { records_.clear(); }

    const SignalRecord* find(std::string_view id) const;
    bool isActive(std::string_view id) const;
    Vec2 value(std::string_view id) const;
    std::optional<Time> heldFor(std::string_view id, Time now) const;

    /// Is any active signal matched by `filter`? Phases in the filter are ignored.
    bool anyActive(const Filter& filter) const;

    const std::map<std::string, SignalRecord, std::less<>>& records() const { return records_; }

private:
    std::map<std::string, SignalRecord, std::less<>> records_;
};

} // namespace gluttony::input
