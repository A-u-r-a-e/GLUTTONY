#include "input/Conditions.hpp"

#include <algorithm>
#include <map>

namespace gluttony::input {

namespace {

std::string formatSeconds(Time t) {
    return std::to_string(static_cast<int>(t * 1000.0 + 0.5)) + "ms";
}

std::vector<ConditionPtr> cloneAll(const std::vector<ConditionPtr>& items) {
    std::vector<ConditionPtr> out;
    out.reserve(items.size());
    for (const auto& item : items) out.push_back(item->clone());
    return out;
}

std::string describeAll(const std::vector<ConditionPtr>& items, std::string_view sep) {
    std::string out;
    for (const auto& item : items) {
        if (!out.empty()) out += sep;
        out += item->describe();
    }
    return out;
}

class OnCondition final : public Condition {
public:
    explicit OnCondition(Filter filter) : filter_(std::move(filter)) {}

    std::optional<Fire> onSignal(const Signal& signal, const InputState&) override {
        if (!filter_.matches(signal)) return std::nullopt;
        return Fire{signal.time, signal.value};
    }
    std::string describe() const override { return filter_.describe(); }
    ConditionPtr clone() const override { return std::make_unique<OnCondition>(*this); }

private:
    Filter filter_;
};

class HoldCondition final : public Condition {
public:
    HoldCondition(Filter filter, Time duration) : filter_(std::move(filter)), duration_(duration) {
        filter_.phases.clear();
    }

    std::optional<Fire> onSignal(const Signal& signal, const InputState&) override {
        // Settle anything that crossed the threshold before this signal arrived.
        std::optional<Fire> fired = check(signal.time);
        if (signal.phase == Phase::Press && filter_.matches(signal))
            pending_.emplace(signal.id, Pending{signal.time, signal.value});
        else if (signal.phase == Phase::Release)
            pending_.erase(signal.id);
        return fired;
    }
    std::optional<Fire> onTick(Time now, const InputState&) override { return check(now); }
    void reset() override { pending_.clear(); }
    float progress() const override { return pending_.empty() ? 0.f : 0.5f; }
    std::string describe() const override {
        return "hold " + filter_.describe() + " " + formatSeconds(duration_);
    }
    ConditionPtr clone() const override { return std::make_unique<HoldCondition>(*this); }

private:
    struct Pending {
        Time since;
        Vec2 value;
    };

    std::optional<Fire> check(Time now) {
        for (auto it = pending_.begin(); it != pending_.end(); ++it) {
            if (now - it->second.since >= duration_) {
                Fire fire{it->second.since + duration_, it->second.value};
                pending_.erase(it);
                return fire;
            }
        }
        return std::nullopt;
    }

    Filter filter_;
    Time duration_;
    std::map<std::string, Pending, std::less<>> pending_;
};

class SequenceCondition final : public Condition {
public:
    SequenceCondition(std::vector<cond::Step> steps, Time gap, std::optional<Filter> breakOn)
        : steps_(std::move(steps)), gap_(gap), breakOn_(std::move(breakOn)) {}

    SequenceCondition(const SequenceCondition& other)
        : gap_(other.gap_), breakOn_(other.breakOn_) {
        for (const auto& step : other.steps_)
            steps_.push_back({step.condition->clone(), step.gap});
    }

    std::optional<Fire> onSignal(const Signal& signal, const InputState& state) override {
        if (steps_.empty()) return std::nullopt;
        if (expired(signal.time)) reset();
        if (current_ > 0 && breakOn_ && breakOn_->matches(signal)) reset();
        if (auto fired = steps_[current_].condition->onSignal(signal, state)) return advance(*fired);
        return std::nullopt;
    }

    std::optional<Fire> onTick(Time now, const InputState& state) override {
        if (steps_.empty()) return std::nullopt;
        if (expired(now)) reset();
        if (auto fired = steps_[current_].condition->onTick(now, state)) return advance(*fired);
        return std::nullopt;
    }

    void reset() override {
        current_ = 0;
        for (auto& step : steps_) step.condition->reset();
    }

    float progress() const override {
        return steps_.empty() ? 0.f : static_cast<float>(current_) / static_cast<float>(steps_.size());
    }

    std::string describe() const override {
        std::string out;
        for (const auto& step : steps_) {
            if (!out.empty()) out += " > ";
            out += step.condition->describe();
        }
        return "[" + out + "] gap " + formatSeconds(gap_);
    }

    ConditionPtr clone() const override { return std::make_unique<SequenceCondition>(*this); }

private:
    Time gapFor(std::size_t step) const { return steps_[step].gap.value_or(gap_); }
    bool expired(Time now) const { return current_ > 0 && now - lastStep_ > gapFor(current_); }

    std::optional<Fire> advance(Fire fired) {
        lastStep_ = fired.time;
        if (++current_ == steps_.size()) {
            reset();
            return fired;
        }
        steps_[current_].condition->reset();
        return std::nullopt;
    }

    std::vector<cond::Step> steps_;
    Time gap_;
    std::optional<Filter> breakOn_;
    std::size_t current_ = 0;
    Time lastStep_ = 0.0;
};

class TogetherCondition final : public Condition {
public:
    TogetherCondition(std::vector<ConditionPtr> parts, Time window)
        : parts_(std::move(parts)), window_(window), fired_(parts_.size()) {}

    TogetherCondition(const TogetherCondition& other)
        : parts_(cloneAll(other.parts_)), window_(other.window_), fired_(parts_.size()) {}

    std::optional<Fire> onSignal(const Signal& signal, const InputState& state) override {
        for (std::size_t i = 0; i < parts_.size(); ++i)
            if (auto f = parts_[i]->onSignal(signal, state)) fired_[i] = f;
        return check(signal.time);
    }

    std::optional<Fire> onTick(Time now, const InputState& state) override {
        for (std::size_t i = 0; i < parts_.size(); ++i)
            if (auto f = parts_[i]->onTick(now, state)) fired_[i] = f;
        return check(now);
    }

    void reset() override {
        for (auto& part : parts_) part->reset();
        std::fill(fired_.begin(), fired_.end(), std::nullopt);
    }

    float progress() const override {
        if (parts_.empty()) return 0.f;
        const auto count = std::count_if(fired_.begin(), fired_.end(),
                                         [](const auto& f) { return f.has_value(); });
        return static_cast<float>(count) / static_cast<float>(parts_.size());
    }

    std::string describe() const override {
        return "{" + describeAll(parts_, " + ") + "} within " + formatSeconds(window_);
    }

    ConditionPtr clone() const override { return std::make_unique<TogetherCondition>(*this); }

private:
    std::optional<Fire> check(Time now) {
        for (auto& f : fired_)
            if (f && now - f->time > window_) f.reset();
        if (parts_.empty() || std::any_of(fired_.begin(), fired_.end(),
                                          [](const auto& f) { return !f.has_value(); }))
            return std::nullopt;
        const Fire latest = **std::max_element(
            fired_.begin(), fired_.end(), [](const auto& a, const auto& b) { return a->time < b->time; });
        reset();
        return latest;
    }

    std::vector<ConditionPtr> parts_;
    Time window_;
    std::vector<std::optional<Fire>> fired_;
};

class AnyOfCondition final : public Condition {
public:
    explicit AnyOfCondition(std::vector<ConditionPtr> parts) : parts_(std::move(parts)) {}
    AnyOfCondition(const AnyOfCondition& other) : parts_(cloneAll(other.parts_)) {}

    std::optional<Fire> onSignal(const Signal& signal, const InputState& state) override {
        return first([&](Condition& c) { return c.onSignal(signal, state); });
    }
    std::optional<Fire> onTick(Time now, const InputState& state) override {
        return first([&](Condition& c) { return c.onTick(now, state); });
    }
    void reset() override {
        for (auto& part : parts_) part->reset();
    }
    float progress() const override {
        float best = 0.f;
        for (const auto& part : parts_) best = std::max(best, part->progress());
        return best;
    }
    std::string describe() const override { return "(" + describeAll(parts_, " / ") + ")"; }
    ConditionPtr clone() const override { return std::make_unique<AnyOfCondition>(*this); }

private:
    template <typename F>
    std::optional<Fire> first(F&& feed) {
        std::optional<Fire> result;
        for (auto& part : parts_) {
            auto fired = feed(*part);
            if (fired && !result) result = fired;
        }
        if (result) reset();
        return result;
    }

    std::vector<ConditionPtr> parts_;
};

class GateCondition final : public Condition {
public:
    GateCondition(ConditionPtr inner, Filter whileActive, bool invert)
        : inner_(std::move(inner)), whileActive_(std::move(whileActive)), invert_(invert) {}
    GateCondition(const GateCondition& other)
        : inner_(other.inner_->clone()), whileActive_(other.whileActive_), invert_(other.invert_) {}

    std::optional<Fire> onSignal(const Signal& signal, const InputState& state) override {
        return pass(inner_->onSignal(signal, state), state);
    }
    std::optional<Fire> onTick(Time now, const InputState& state) override {
        return pass(inner_->onTick(now, state), state);
    }
    void reset() override { inner_->reset(); }
    float progress() const override { return inner_->progress(); }
    std::string describe() const override {
        return inner_->describe() + (invert_ ? " unless " : " while ") + whileActive_.describe();
    }
    ConditionPtr clone() const override { return std::make_unique<GateCondition>(*this); }

private:
    std::optional<Fire> pass(std::optional<Fire> fired, const InputState& state) const {
        if (fired && state.anyActive(whileActive_) != invert_) return fired;
        return std::nullopt;
    }

    ConditionPtr inner_;
    Filter whileActive_;
    bool invert_;
};

} // namespace

namespace cond {

ConditionPtr on(Filter filter) { return std::make_unique<OnCondition>(std::move(filter)); }

ConditionPtr hold(Filter filter, Time duration) {
    return std::make_unique<HoldCondition>(std::move(filter), duration);
}

ConditionPtr sequence(std::vector<Step> steps, Time gap, std::optional<Filter> breakOn) {
    return std::make_unique<SequenceCondition>(std::move(steps), gap, std::move(breakOn));
}

ConditionPtr sequence(std::vector<ConditionPtr> steps, Time gap) {
    std::vector<Step> wrapped;
    for (auto& step : steps) wrapped.push_back({std::move(step), std::nullopt});
    return sequence(std::move(wrapped), gap);
}

ConditionPtr together(std::vector<ConditionPtr> parts, Time window) {
    return std::make_unique<TogetherCondition>(std::move(parts), window);
}

ConditionPtr anyOf(std::vector<ConditionPtr> parts) {
    return std::make_unique<AnyOfCondition>(std::move(parts));
}

ConditionPtr gate(ConditionPtr inner, Filter whileActive, bool invert) {
    return std::make_unique<GateCondition>(std::move(inner), std::move(whileActive), invert);
}

} // namespace cond

} // namespace gluttony::input
