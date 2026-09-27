#pragma once

#include "input/Signal.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace gluttony::input {

/// Glob match supporting '*' (any run) and '?' (any one char).
bool globMatch(std::string_view pattern, std::string_view text);

/// Picks things by id pattern and tags. Used for signals and for channels.
/// Every empty list means "no constraint".
struct Selector {
    std::vector<std::string> ids;      ///< glob patterns; match if any matches
    std::vector<std::string> allTags;  ///< must have every one
    std::vector<std::string> anyTags;  ///< must have at least one
    std::vector<std::string> noneTags; ///< must have none

    bool matches(std::string_view id, const std::vector<std::string>& tags) const;
    std::string describe() const;
};

/// A condition on a single signal.
struct Filter {
    Selector select;
    std::vector<Phase> phases;         ///< empty = any phase
    std::optional<float> minMagnitude;
    std::optional<float> maxMagnitude;
    std::optional<float> angle;        ///< degrees; value direction must be within arc/2
    float arc = 90.f;

    bool matches(const Signal& signal) const;
    std::string describe() const;

    static Filter id(std::string pattern, std::vector<Phase> phases = {});
    static Filter tag(std::string tag, std::vector<Phase> phases = {});
};

} // namespace gluttony::input
