#include "input/Mute.hpp"

#include "input/Channel.hpp"

#include <iterator>

namespace gluttony::input {

MuteId MuteTable::add(MuteRule rule) {
    const MuteId id = next_++;
    rules_.emplace(id, std::move(rule));
    return id;
}

bool MuteTable::remove(MuteId id) { return rules_.erase(id) > 0; }

void MuteTable::expire(Time now) {
    std::erase_if(rules_, [now](const auto& entry) {
        return entry.second.until && now >= *entry.second.until;
    });
}

bool MuteTable::mutesPipeline(const Signal& signal) const {
    for (const auto& [id, rule] : rules_)
        if (!rule.channels && (!rule.signals || rule.signals->matches(signal))) return true;
    return false;
}

bool MuteTable::mutesChannel(const Channel& channel) const {
    for (const auto& [id, rule] : rules_)
        if (rule.channels && !rule.signals && rule.channels->matches(channel.name(), channel.tags()))
            return true;
    return false;
}

bool MuteTable::mutesChannel(const Channel& channel, const Signal& signal) const {
    for (const auto& [id, rule] : rules_)
        if (rule.channels && rule.signals && rule.channels->matches(channel.name(), channel.tags()) &&
            rule.signals->matches(signal))
            return true;
    return false;
}

} // namespace gluttony::input
