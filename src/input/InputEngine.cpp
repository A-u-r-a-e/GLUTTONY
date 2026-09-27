#include "input/InputEngine.hpp"

#include <algorithm>

namespace gluttony::input {

InputEngine::InputEngine() : InputEngine(Options{}) {}

InputEngine::InputEngine(Options options) : options_(options) {}

void InputEngine::feed(Signal signal) {
    queue_.push_back(std::move(signal));
    drain();
}

void InputEngine::update(Time now) {
    now_ = std::max(now_, now);
    mutes_.expire(now_);

    std::vector<Signal> out;
    for (auto& entry : processors_)
        if (entry.enabled) entry.processor->onTick(now_, state_, out);
    for (auto& signal : out) queue_.push_back(std::move(signal));

    for (auto& channel : channels_) {
        if (!channel->enabled()) continue;
        if (mutes_.mutesChannel(*channel)) {
            channel->reset();
            continue;
        }
        if (auto fired = channel->onTick(now_, state_)) emitFire(*channel, *fired, 1);
    }
    drain();
}

void InputEngine::drain() {
    if (draining_) return; // re-entrant feed from a callback: the outer loop picks it up
    draining_ = true;
    while (!queue_.empty()) {
        Signal signal = std::move(queue_.front());
        queue_.pop_front();
        process(signal);
    }
    draining_ = false;
}

void InputEngine::process(const Signal& signal) {
    if (signal.depth > options_.maxDepth) return;
    now_ = std::max(now_, signal.time);
    mutes_.expire(now_);
    state_.apply(signal);

    const bool muted = mutes_.mutesPipeline(signal);
    log_.push_back({signal, muted});
    while (log_.size() > options_.logSize) log_.pop_front();
    if (muted) return;

    // Copy: a callback may add or remove listeners.
    std::vector<std::function<void(const Signal&)>> callbacks;
    for (const auto& [id, listener] : listeners_)
        if (listener.filter.matches(signal)) callbacks.push_back(listener.callback);
    for (const auto& callback : callbacks) callback(signal);

    std::vector<Signal> out;
    for (auto& entry : processors_)
        if (entry.enabled) entry.processor->onSignal(signal, state_, out);
    for (auto& derived : out) queue_.push_back(std::move(derived));

    for (auto& channel : channels_) {
        if (!channel->enabled()) continue;
        if (mutes_.mutesChannel(*channel)) {
            channel->reset();
            continue;
        }
        if (mutes_.mutesChannel(*channel, signal)) continue;
        if (auto fired = channel->onSignal(signal, state_)) emitFire(*channel, *fired, signal.depth + 1);
    }
}

void InputEngine::emitFire(const Channel& channel, const Fire& fire, int depth) {
    if (!channel.emits()) return;
    std::vector<std::string> tags{"channel"};
    tags.insert(tags.end(), channel.tags().begin(), channel.tags().end());
    queue_.push_back(Signal{channel.signalId(), Phase::Pulse, fire.value, fire.time, std::move(tags), depth});
}

Processor& InputEngine::addProcessor(std::string name, std::unique_ptr<Processor> processor) {
    processors_.push_back({std::move(name), std::move(processor), true});
    return *processors_.back().processor;
}

bool InputEngine::removeProcessor(std::string_view name) {
    return std::erase_if(processors_, [&](const auto& e) { return e.name == name; }) > 0;
}

void InputEngine::setProcessorEnabled(std::string_view name, bool enabled) {
    for (auto& entry : processors_) {
        if (entry.name != name) continue;
        if (!enabled) entry.processor->reset();
        entry.enabled = enabled;
    }
}

Channel& InputEngine::addChannel(ChannelSpec spec) {
    channels_.push_back(std::make_unique<Channel>(std::move(spec)));
    return *channels_.back();
}

Channel* InputEngine::channel(std::string_view name) {
    for (auto& channel : channels_)
        if (channel->name() == name) return channel.get();
    return nullptr;
}

bool InputEngine::removeChannel(std::string_view name) {
    return std::erase_if(channels_, [&](const auto& c) { return c->name() == name; }) > 0;
}

ListenerId InputEngine::listen(Filter filter, std::function<void(const Signal&)> callback) {
    const ListenerId id = nextListener_++;
    listeners_.emplace(id, Listener{std::move(filter), std::move(callback)});
    return id;
}

void InputEngine::unlisten(ListenerId id) { listeners_.erase(id); }

void InputEngine::clear() {
    processors_.clear();
    channels_.clear();
    mutes_.clear();
    state_.clear();
    queue_.clear();
    log_.clear();
}

} // namespace gluttony::input
