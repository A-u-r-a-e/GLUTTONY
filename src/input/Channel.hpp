#pragma once

#include "input/Conditions.hpp"

#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace gluttony::input {

class Channel;

struct ChannelSpec {
    std::string name;
    std::vector<std::string> tags; ///< classes, e.g. for muting groups of channels
    ConditionPtr when;
    Time cooldown = 0.0;
    bool enabled = true;
    bool emit = true; ///< feed a "channel/<name>" pulse back into the engine when it fires
    std::function<void(const Channel&, const Fire&)> onFire;
};

/// Watches the signal stream for one configurable condition.
class Channel {
public:
    explicit Channel(ChannelSpec spec);

    const std::string& name() const { return spec_.name; }
    const std::vector<std::string>& tags() const { return spec_.tags; }
    std::string signalId() const { return "channel/" + spec_.name; }
    bool emits() const { return spec_.emit; }

    bool enabled() const { return spec_.enabled; }
    void setEnabled(bool enabled);
    void setOnFire(std::function<void(const Channel&, const Fire&)> callback);

    const Condition& condition() const { return *spec_.when; }
    int fireCount() const { return fireCount_; }
    std::optional<Time> lastFire() const { return lastFire_; }

    // Driven by the engine.
    std::optional<Fire> onSignal(const Signal& signal, const InputState& state);
    std::optional<Fire> onTick(Time now, const InputState& state);
    void reset();

private:
    std::optional<Fire> accept(std::optional<Fire> fired);

    ChannelSpec spec_;
    int fireCount_ = 0;
    std::optional<Time> lastFire_;
};

} // namespace gluttony::input
