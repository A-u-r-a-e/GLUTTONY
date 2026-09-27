#include "input/Channel.hpp"

#include <stdexcept>

namespace gluttony::input {

Channel::Channel(ChannelSpec spec) : spec_(std::move(spec)) {
    if (!spec_.when) throw std::invalid_argument("channel '" + spec_.name + "' has no condition");
}

void Channel::setEnabled(bool enabled) {
    if (!enabled) spec_.when->reset();
    spec_.enabled = enabled;
}

void Channel::setOnFire(std::function<void(const Channel&, const Fire&)> callback) {
    spec_.onFire = std::move(callback);
}

std::optional<Fire> Channel::onSignal(const Signal& signal, const InputState& state) {
    return accept(spec_.when->onSignal(signal, state));
}

std::optional<Fire> Channel::onTick(Time now, const InputState& state) {
    return accept(spec_.when->onTick(now, state));
}

void Channel::reset() { spec_.when->reset(); }

std::optional<Fire> Channel::accept(std::optional<Fire> fired) {
    if (!fired) return std::nullopt;
    if (lastFire_ && fired->time - *lastFire_ < spec_.cooldown) return std::nullopt;
    lastFire_ = fired->time;
    ++fireCount_;
    if (spec_.onFire) spec_.onFire(*this, *fired);
    return fired;
}

} // namespace gluttony::input
