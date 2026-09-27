#include "input/Processors.hpp"

#include <cmath>

namespace gluttony::input {

namespace {

float wrap360(float degrees) {
    float d = std::fmod(degrees, 360.f);
    return d < 0.f ? d + 360.f : d;
}

} // namespace

// --- Bind -------------------------------------------------------------------

Bind::Bind(Filter from, std::string to, std::vector<std::string> tags)
    : from_(std::move(from)), to_(std::move(to)), tags_(std::move(tags)) {}

void Bind::onSignal(const Signal& in, const InputState&, std::vector<Signal>& out) {
    if (!from_.matches(in)) return;
    switch (in.phase) {
    case Phase::Press:
        if (active_.insert(in.id).second && active_.size() == 1)
            out.push_back(Signal::derived(in, to_, Phase::Press, in.value, tags_));
        break;
    case Phase::Release:
        if (active_.erase(in.id) && active_.empty())
            out.push_back(Signal::derived(in, to_, Phase::Release, in.value, tags_));
        break;
    case Phase::Change:
    case Phase::Pulse:
        out.push_back(Signal::derived(in, to_, in.phase, in.value, tags_));
        break;
    }
}

std::string Bind::describe() const { return from_.describe() + " -> " + to_; }

// --- Axis2D -----------------------------------------------------------------

Axis2D::Axis2D(Spec spec) : spec_(std::move(spec)) {
    for (Filter* f : {&spec_.up, &spec_.down, &spec_.left, &spec_.right}) f->phases.clear();
}

void Axis2D::onSignal(const Signal& in, const InputState&, std::vector<Signal>& out) {
    if (in.phase != Phase::Press && in.phase != Phase::Release) return;
    const Filter* filters[4] = {&spec_.up, &spec_.down, &spec_.left, &spec_.right};
    bool touched = false;
    for (int i = 0; i < 4; ++i) {
        if (!filters[i]->matches(in)) continue;
        touched = true;
        if (in.phase == Phase::Press)
            held_[i].insert(in.id);
        else
            held_[i].erase(in.id);
    }
    if (!touched) return;

    auto on = [&](int i) { return held_[i].empty() ? 0.f : 1.f; };
    Vec2 v{on(3) - on(2), on(1) - on(0)};
    if (spec_.normalize) v = v.normalized();
    if (v == current_) return;

    const Phase phase = current_ == Vec2{} ? Phase::Press : v == Vec2{} ? Phase::Release : Phase::Change;
    current_ = v;
    out.push_back(Signal::derived(in, spec_.to, phase, v, spec_.tags));
}

void Axis2D::reset() {
    for (auto& held : held_) held.clear();
    current_ = {};
}

std::string Axis2D::describe() const { return "axis2d -> " + spec_.to; }

// --- Relative ---------------------------------------------------------------

Relative::Relative(Spec spec) : spec_(std::move(spec)) {}

void Relative::onSignal(const Signal& in, const InputState&, std::vector<Signal>& out) {
    if (in.id == spec_.from)
        from_ = in.value;
    else if (spec_.origin && in.id == *spec_.origin)
        origin_ = in.value;
    else
        return;

    const Vec2 v = from_ - origin_;
    if (last_ && *last_ == v) return;
    last_ = v;
    out.push_back(Signal::derived(in, spec_.to, Phase::Change, v, spec_.tags));
}

void Relative::reset() {
    from_ = origin_ = {};
    last_.reset();
}

std::string Relative::describe() const {
    return spec_.from + (spec_.origin ? " - " + *spec_.origin : "") + " -> " + spec_.to;
}

// --- Sectors ----------------------------------------------------------------

Sectors::Sectors(Spec spec) : spec_(std::move(spec)) {}

std::optional<std::size_t> Sectors::sectorOf(Vec2 v) const {
    const float length = v.length();
    if (spec_.names.empty() || length == 0.f || length <= spec_.deadzone) return std::nullopt;
    const float width = 360.f / static_cast<float>(spec_.names.size());
    const float rel = wrap360(v.angleDegrees() - reference_.angleDegrees() - spec_.offsetDegrees +
                              width / 2.f);
    return static_cast<std::size_t>(rel / width) % spec_.names.size();
}

void Sectors::onSignal(const Signal& in, const InputState&, std::vector<Signal>& out) {
    if (spec_.reference && in.id == *spec_.reference) {
        if (in.value.length() > 0.f) reference_ = in.value;
        if (sourceActive_) moveTo(sectorOf(source_), in, out);
        return;
    }
    if (in.id != spec_.source) return;

    switch (in.phase) {
    case Phase::Pulse:
        if (auto sector = sectorOf(in.value))
            out.push_back(Signal::derived(in, idFor(*sector), Phase::Pulse, in.value.normalized(),
                                          spec_.tags));
        break;
    case Phase::Release:
        sourceActive_ = false;
        moveTo(std::nullopt, in, out);
        break;
    case Phase::Press:
    case Phase::Change:
        source_ = in.value;
        sourceActive_ = true;
        moveTo(sectorOf(source_), in, out);
        break;
    }
}

void Sectors::moveTo(std::optional<std::size_t> sector, const Signal& cause, std::vector<Signal>& out) {
    if (sector == current_) return;
    if (current_)
        out.push_back(Signal::derived(cause, idFor(*current_), Phase::Release, {}, spec_.tags));
    if (sector)
        out.push_back(Signal::derived(cause, idFor(*sector), Phase::Press, source_.normalized(),
                                      spec_.tags));
    current_ = sector;
}

void Sectors::reset() {
    reference_ = {1.f, 0.f};
    source_ = {};
    sourceActive_ = false;
    current_.reset();
}

std::string Sectors::describe() const {
    return spec_.source + (spec_.reference ? " rel " + *spec_.reference : "") + " -> " + spec_.to +
           "/{" + std::to_string(spec_.names.size()) + "}";
}

// --- Flick ------------------------------------------------------------------

Flick::Flick(Spec spec) : spec_(std::move(spec)) {}

void Flick::onSignal(const Signal& in, const InputState&, std::vector<Signal>& out) {
    if (in.id != spec_.source || (in.phase != Phase::Change && in.phase != Phase::Pulse)) return;

    history_.push_back({in.time, in.value});
    while (history_.size() > 1 && history_.front().time < in.time - spec_.window) history_.pop_front();

    const Time span = history_.back().time - history_.front().time;
    if (span < spec_.minSpan || span <= 0.0) return;

    const Vec2 velocity = (history_.back().pos - history_.front().pos) * static_cast<float>(1.0 / span);
    if (velocity.length() < spec_.minSpeed) return;
    if (lastFlick_ && in.time - *lastFlick_ < spec_.cooldown) return;

    lastFlick_ = in.time;
    history_.clear();
    history_.push_back({in.time, in.value});
    out.push_back(Signal::derived(in, spec_.to, Phase::Pulse, velocity, spec_.tags));
}

void Flick::reset() {
    history_.clear();
    lastFlick_.reset();
}

std::string Flick::describe() const { return "flick " + spec_.source + " -> " + spec_.to; }

// --- Threshold --------------------------------------------------------------

Threshold::Threshold(Spec spec) : spec_(std::move(spec)) {}

void Threshold::onSignal(const Signal& in, const InputState&, std::vector<Signal>& out) {
    if (!spec_.from.matches(in)) return;
    const float amount = spec_.useMagnitude ? in.value.length() : in.value.x;
    if (!active_ && amount >= spec_.pressAt) {
        active_ = true;
        out.push_back(Signal::derived(in, spec_.to, Phase::Press, in.value, spec_.tags));
    } else if (active_ && amount < spec_.releaseAt) {
        active_ = false;
        out.push_back(Signal::derived(in, spec_.to, Phase::Release, in.value, spec_.tags));
    }
}

std::string Threshold::describe() const { return "threshold " + spec_.from.describe() + " -> " + spec_.to; }

} // namespace gluttony::input
