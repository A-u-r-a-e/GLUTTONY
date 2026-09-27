#include "input/Filter.hpp"

#include <algorithm>

namespace gluttony::input {

bool globMatch(std::string_view pattern, std::string_view text) {
    std::size_t p = 0, t = 0;
    std::size_t starP = std::string_view::npos, starT = 0;
    while (t < text.size()) {
        if (p < pattern.size() && (pattern[p] == '?' || pattern[p] == text[t])) {
            ++p;
            ++t;
        } else if (p < pattern.size() && pattern[p] == '*') {
            starP = p++;
            starT = t;
        } else if (starP != std::string_view::npos) {
            p = starP + 1;
            t = ++starT;
        } else {
            return false;
        }
    }
    while (p < pattern.size() && pattern[p] == '*') ++p;
    return p == pattern.size();
}

namespace {

bool contains(const std::vector<std::string>& tags, const std::string& tag) {
    return std::find(tags.begin(), tags.end(), tag) != tags.end();
}

std::string join(const std::vector<std::string>& items, std::string_view sep) {
    std::string out;
    for (const auto& item : items) {
        if (!out.empty()) out += sep;
        out += item;
    }
    return out;
}

} // namespace

bool Selector::matches(std::string_view id, const std::vector<std::string>& tags) const {
    if (!ids.empty() && std::none_of(ids.begin(), ids.end(),
                                     [&](const std::string& p) { return globMatch(p, id); }))
        return false;
    for (const auto& tag : allTags)
        if (!contains(tags, tag)) return false;
    if (!anyTags.empty() && std::none_of(anyTags.begin(), anyTags.end(),
                                         [&](const std::string& t) { return contains(tags, t); }))
        return false;
    for (const auto& tag : noneTags)
        if (contains(tags, tag)) return false;
    return true;
}

std::string Selector::describe() const {
    std::string out = ids.empty() ? "*" : join(ids, "|");
    if (!allTags.empty()) out += " #" + join(allTags, "&");
    if (!anyTags.empty()) out += " #" + join(anyTags, "|");
    if (!noneTags.empty()) out += " !#" + join(noneTags, "|");
    return out;
}

bool Filter::matches(const Signal& signal) const {
    if (!phases.empty() && std::find(phases.begin(), phases.end(), signal.phase) == phases.end())
        return false;
    if (!select.matches(signal.id, signal.tags)) return false;
    const float mag = signal.value.length();
    if (minMagnitude && mag < *minMagnitude) return false;
    if (maxMagnitude && mag > *maxMagnitude) return false;
    if (angle) {
        if (mag == 0.f) return false;
        if (std::abs(angleDelta(signal.value.angleDegrees(), *angle)) > arc / 2.f) return false;
    }
    return true;
}

std::string Filter::describe() const {
    std::string out = select.describe();
    if (!phases.empty()) {
        out += " (";
        for (std::size_t i = 0; i < phases.size(); ++i) {
            if (i) out += "|";
            out += phaseName(phases[i]);
        }
        out += ")";
    }
    return out;
}

Filter Filter::id(std::string pattern, std::vector<Phase> phases) {
    Filter f;
    f.select.ids.push_back(std::move(pattern));
    f.phases = std::move(phases);
    return f;
}

Filter Filter::tag(std::string tag, std::vector<Phase> phases) {
    Filter f;
    f.select.allTags.push_back(std::move(tag));
    f.phases = std::move(phases);
    return f;
}

} // namespace gluttony::input
