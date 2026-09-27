#pragma once

#include "input/Filter.hpp"

#include <cstdint>
#include <map>
#include <optional>
#include <string>

namespace gluttony::input {

class Channel;

using MuteId = std::uint32_t;

/// One mute. Rules stack; each is removed independently.
///
///  signals | channels | effect
///  --------+----------+-------------------------------------------------------------
///  set     | unset    | matching signals are dropped from the whole pipeline
///  unset   | set      | matching channels are suspended entirely
///  set     | set      | matching channels stop seeing matching signals
///  unset   | unset    | everything is muted
struct MuteRule {
    std::string label;
    std::optional<Filter> signals;
    std::optional<Selector> channels;
    std::optional<Time> until; ///< expires automatically at this time
};

class MuteTable {
public:
    MuteId add(MuteRule rule);
    bool remove(MuteId id);
    void clear() { rules_.clear(); }
    void expire(Time now);

    /// Is this signal dropped before anything reacts to it?
    bool mutesPipeline(const Signal& signal) const;
    /// Is this channel suspended entirely?
    bool mutesChannel(const Channel& channel) const;
    /// Is this signal hidden from this channel?
    bool mutesChannel(const Channel& channel, const Signal& signal) const;

    const std::map<MuteId, MuteRule>& rules() const { return rules_; }

private:
    std::map<MuteId, MuteRule> rules_;
    MuteId next_ = 1;
};

} // namespace gluttony::input
