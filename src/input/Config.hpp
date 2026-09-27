#pragma once

#include "input/Conditions.hpp"
#include "input/Filter.hpp"
#include "input/InputEngine.hpp"
#include "input/Mute.hpp"

#include <nlohmann/json_fwd.hpp>

#include <filesystem>
#include <string>
#include <vector>

namespace gluttony::input::config {

/// A named mute preset the host can switch on and off (e.g. from a debug key).
struct MuteGroup {
    std::string name;
    std::vector<std::string> toggle; ///< signal ids the host may use to toggle it
    MuteRule rule;
};

struct Loaded {
    std::vector<MuteGroup> muteGroups;
};

/// Adds everything described in the json to the engine. Throws std::runtime_error with
/// a path to the offending entry on bad input. See data/input.json for the format.
Loaded apply(const nlohmann::json& root, InputEngine& engine);
Loaded loadFile(const std::filesystem::path& path, InputEngine& engine);

/// Accepts "pattern", ["a", "b"], or an object with id/tags/phase/... keys.
Filter parseFilter(const nlohmann::json& j, std::vector<Phase> defaultPhases = {});
Selector parseSelector(const nlohmann::json& j);
ConditionPtr parseCondition(const nlohmann::json& j);
MuteRule parseMute(const nlohmann::json& j);

} // namespace gluttony::input::config
