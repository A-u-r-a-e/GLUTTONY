#include "input/InputState.hpp"

namespace gluttony::input {

void InputState::apply(const Signal& signal) {
    auto it = records_.find(signal.id);
    if (it == records_.end()) it = records_.emplace(signal.id, SignalRecord{}).first;
    SignalRecord& record = it->second;
    record.last = signal;
    if (signal.phase == Phase::Press && !record.active) {
        record.active = true;
        record.activeSince = signal.time;
    } else if (signal.phase == Phase::Release) {
        record.active = false;
    }
}

const SignalRecord* InputState::find(std::string_view id) const {
    auto it = records_.find(id);
    return it == records_.end() ? nullptr : &it->second;
}

bool InputState::isActive(std::string_view id) const {
    const SignalRecord* record = find(id);
    return record && record->active;
}

Vec2 InputState::value(std::string_view id) const {
    const SignalRecord* record = find(id);
    return record ? record->last.value : Vec2{};
}

std::optional<Time> InputState::heldFor(std::string_view id, Time now) const {
    const SignalRecord* record = find(id);
    if (!record || !record->active) return std::nullopt;
    return now - record->activeSince;
}

bool InputState::anyActive(const Filter& filter) const {
    Filter ignoringPhase = filter;
    ignoringPhase.phases.clear();
    for (const auto& [id, record] : records_)
        if (record.active && ignoringPhase.matches(record.last)) return true;
    return false;
}

} // namespace gluttony::input
