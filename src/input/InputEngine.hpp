#pragma once

#include "input/Channel.hpp"
#include "input/Filter.hpp"
#include "input/InputState.hpp"
#include "input/Mute.hpp"
#include "input/Processors.hpp"
#include "input/Signal.hpp"

#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace gluttony::input {

struct LogEntry {
    Signal signal;
    bool muted = false;
};

using ListenerId = std::uint32_t;

/// The hub. Feed it signals from anywhere (platform input, game context, other systems)
/// and advance time with update(). Signals flow:
///
///   feed -> state -> mutes -> listeners -> processors -> channels
///                                             |            |
///                                             +-- derived signals fed back in --+
class InputEngine {
public:
    struct Options {
        int maxDepth = 16;         ///< guards against derivation loops
        std::size_t logSize = 512; ///< recent signals kept for debugging
    };

    InputEngine();
    explicit InputEngine(Options options);

    void feed(Signal signal);
    void update(Time now);
    Time now() const { return now_; }

    // Processors -------------------------------------------------------------
    Processor& addProcessor(std::string name, std::unique_ptr<Processor> processor);
    bool removeProcessor(std::string_view name);
    void setProcessorEnabled(std::string_view name, bool enabled);

    struct ProcessorEntry {
        std::string name;
        std::unique_ptr<Processor> processor;
        bool enabled = true;
    };
    const std::vector<ProcessorEntry>& processors() const { return processors_; }

    // Channels ---------------------------------------------------------------
    Channel& addChannel(ChannelSpec spec);
    Channel* channel(std::string_view name);
    bool removeChannel(std::string_view name);
    const std::vector<std::unique_ptr<Channel>>& channels() const { return channels_; }

    // Mutes ------------------------------------------------------------------
    MuteId mute(MuteRule rule) { return mutes_.add(std::move(rule)); }
    bool unmute(MuteId id) { return mutes_.remove(id); }
    const MuteTable& mutes() const { return mutes_; }
    bool isChannelMuted(const Channel& channel) const { return mutes_.mutesChannel(channel); }

    // Listening --------------------------------------------------------------
    /// Called for every unmuted signal matching the filter (channel fires included).
    ListenerId listen(Filter filter, std::function<void(const Signal&)> callback);
    void unlisten(ListenerId id);

    // Introspection ----------------------------------------------------------
    const InputState& state() const { return state_; }
    const std::deque<LogEntry>& log() const { return log_; }

    /// Drops processors, channels, mutes and state. Listeners are kept.
    void clear();

private:
    struct Listener {
        Filter filter;
        std::function<void(const Signal&)> callback;
    };

    void drain();
    void process(const Signal& signal);
    void emitFire(const Channel& channel, const Fire& fire, int depth);

    Options options_;
    Time now_ = 0.0;
    InputState state_;
    MuteTable mutes_;
    std::vector<ProcessorEntry> processors_;
    std::vector<std::unique_ptr<Channel>> channels_;
    std::map<ListenerId, Listener> listeners_;
    ListenerId nextListener_ = 1;
    std::deque<Signal> queue_;
    std::deque<LogEntry> log_;
    bool draining_ = false;
};

} // namespace gluttony::input
