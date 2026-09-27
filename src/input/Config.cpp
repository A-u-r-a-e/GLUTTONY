#include "input/Config.hpp"

#include <nlohmann/json.hpp>

#include <fstream>
#include <stdexcept>

namespace gluttony::input::config {

using nlohmann::json;

namespace {

[[noreturn]] void fail(const std::string& path, const std::string& message) {
    throw std::runtime_error("input config" + (path.empty() ? "" : " at " + path) + ": " + message);
}

std::vector<std::string> stringList(const json& j, const std::string& path) {
    if (j.is_string()) return {j.get<std::string>()};
    if (!j.is_array()) fail(path, "expected a string or array of strings");
    std::vector<std::string> out;
    for (std::size_t i = 0; i < j.size(); ++i) {
        if (!j[i].is_string()) fail(path + "[" + std::to_string(i) + "]", "expected a string");
        out.push_back(j[i].get<std::string>());
    }
    return out;
}

std::vector<std::string> optionalStrings(const json& obj, const char* key, const std::string& path) {
    return obj.contains(key) ? stringList(obj[key], path + "." + key) : std::vector<std::string>{};
}

std::string requiredString(const json& obj, const char* key, const std::string& path) {
    if (!obj.contains(key) || !obj[key].is_string()) fail(path, std::string("needs a string '") + key + "'");
    return obj[key].get<std::string>();
}

std::optional<std::string> optionalString(const json& obj, const char* key) {
    if (obj.contains(key) && obj[key].is_string()) return obj[key].get<std::string>();
    return std::nullopt;
}

template <typename T>
T number(const json& obj, const char* key, T fallback, const std::string& path) {
    if (!obj.contains(key)) return fallback;
    if (!obj[key].is_number()) fail(path + "." + key, "expected a number");
    return obj[key].get<T>();
}

std::vector<Phase> parsePhases(const json& j, const std::string& path) {
    std::vector<Phase> out;
    for (const auto& name : stringList(j, path)) {
        if (name == "any") return {};
        auto phase = phaseFromString(name);
        if (!phase) fail(path, "unknown phase '" + name + "' (press, release, change, pulse, any)");
        out.push_back(*phase);
    }
    return out;
}

Selector parseSelectorAt(const json& j, const std::string& path) {
    Selector s;
    if (j.is_string() || j.is_array()) {
        s.ids = stringList(j, path);
        return s;
    }
    if (!j.is_object()) fail(path, "expected a pattern, list of patterns, or object");
    s.ids = optionalStrings(j, "id", path);
    auto all = optionalStrings(j, "tags", path);
    auto allToo = optionalStrings(j, "allTags", path);
    all.insert(all.end(), allToo.begin(), allToo.end());
    s.allTags = std::move(all);
    s.anyTags = optionalStrings(j, "anyTags", path);
    s.noneTags = optionalStrings(j, "noneTags", path);
    return s;
}

/// Reads filter keys from `obj` on top of `f`. `obj` may also hold other (condition) keys.
void applyFilterKeys(Filter& f, const json& obj, const std::string& path) {
    Selector extra = parseSelectorAt(obj, path);
    f.select.ids.insert(f.select.ids.end(), extra.ids.begin(), extra.ids.end());
    f.select.allTags.insert(f.select.allTags.end(), extra.allTags.begin(), extra.allTags.end());
    f.select.anyTags.insert(f.select.anyTags.end(), extra.anyTags.begin(), extra.anyTags.end());
    f.select.noneTags.insert(f.select.noneTags.end(), extra.noneTags.begin(), extra.noneTags.end());
    if (obj.contains("phase")) f.phases = parsePhases(obj["phase"], path + ".phase");
    if (obj.contains("minMagnitude")) f.minMagnitude = number<float>(obj, "minMagnitude", 0.f, path);
    if (obj.contains("maxMagnitude")) f.maxMagnitude = number<float>(obj, "maxMagnitude", 0.f, path);
    if (obj.contains("angle")) f.angle = number<float>(obj, "angle", 0.f, path);
    f.arc = number<float>(obj, "arc", f.arc, path);
}

Filter parseFilterAt(const json& j, std::vector<Phase> defaultPhases, const std::string& path) {
    Filter f;
    f.phases = std::move(defaultPhases);
    if (j.is_string() || j.is_array())
        f.select.ids = stringList(j, path);
    else if (j.is_object())
        applyFilterKeys(f, j, path);
    else
        fail(path, "expected a pattern, list of patterns, or filter object");
    return f;
}

const std::vector<Phase> kTriggerPhases{Phase::Press, Phase::Pulse};

ConditionPtr parseConditionAt(const json& j, const std::string& path);

std::vector<ConditionPtr> parseConditionList(const json& j, const std::string& path) {
    if (!j.is_array() || j.empty()) fail(path, "expected a non-empty array of conditions");
    std::vector<ConditionPtr> out;
    for (std::size_t i = 0; i < j.size(); ++i)
        out.push_back(parseConditionAt(j[i], path + "[" + std::to_string(i) + "]"));
    return out;
}

ConditionPtr parseConditionAt(const json& j, const std::string& path) {
    if (j.is_string()) return cond::on(parseFilterAt(j, kTriggerPhases, path));
    if (!j.is_object()) fail(path, "expected a condition (string or object)");

    ConditionPtr result;
    if (j.contains("on")) {
        Filter f = parseFilterAt(j["on"], kTriggerPhases, path + ".on");
        json rest = j;
        for (const char* key : {"on", "id", "while", "unless", "gap"}) rest.erase(key);
        applyFilterKeys(f, rest, path);
        result = cond::on(std::move(f));
    } else if (j.contains("hold")) {
        result = cond::hold(parseFilterAt(j["hold"], {}, path + ".hold"),
                            number<Time>(j, "for", 0.5, path));
    } else if (j.contains("seq")) {
        const json& items = j["seq"];
        if (!items.is_array() || items.empty()) fail(path + ".seq", "expected a non-empty array");
        std::vector<cond::Step> steps;
        for (std::size_t i = 0; i < items.size(); ++i) {
            const std::string stepPath = path + ".seq[" + std::to_string(i) + "]";
            std::optional<Time> gap;
            if (items[i].is_object() && items[i].contains("gap")) gap = number<Time>(items[i], "gap", 0.0, stepPath);
            steps.push_back({parseConditionAt(items[i], stepPath), gap});
        }
        std::optional<Filter> breakOn;
        if (j.contains("breakOn")) breakOn = parseFilterAt(j["breakOn"], {}, path + ".breakOn");
        result = cond::sequence(std::move(steps), number<Time>(j, "gap", 0.4, path), std::move(breakOn));
    } else if (j.contains("together")) {
        result = cond::together(parseConditionList(j["together"], path + ".together"),
                                number<Time>(j, "window", 0.08, path));
    } else if (j.contains("any")) {
        result = cond::anyOf(parseConditionList(j["any"], path + ".any"));
    } else {
        fail(path, "condition needs one of: on, hold, seq, together, any");
    }

    if (j.contains("while"))
        result = cond::gate(std::move(result), parseFilterAt(j["while"], {}, path + ".while"));
    if (j.contains("unless"))
        result = cond::gate(std::move(result), parseFilterAt(j["unless"], {}, path + ".unless"), true);
    return result;
}

MuteRule parseMuteAt(const json& j, const std::string& path) {
    if (!j.is_object()) fail(path, "expected an object");
    MuteRule rule;
    rule.label = optionalString(j, "label").value_or(optionalString(j, "name").value_or(""));
    if (j.contains("signals")) rule.signals = parseFilterAt(j["signals"], {}, path + ".signals");
    if (j.contains("channels")) rule.channels = parseSelectorAt(j["channels"], path + ".channels");
    if (j.contains("until")) rule.until = number<Time>(j, "until", 0.0, path);
    return rule;
}

std::unique_ptr<Processor> parseProcessor(const json& j, const std::string& path) {
    const std::string type = requiredString(j, "type", path);
    const std::string to = requiredString(j, "to", path);
    auto tags = optionalStrings(j, "tags", path);

    if (type == "bind") {
        if (!j.contains("from")) fail(path, "bind needs 'from'");
        return std::make_unique<Bind>(parseFilterAt(j["from"], {}, path + ".from"), to, tags);
    }
    if (type == "axis2d") {
        Axis2D::Spec spec{to, tags, {}, {}, {}, {}, j.value("normalize", true)};
        Filter* sides[4] = {&spec.up, &spec.down, &spec.left, &spec.right};
        const char* keys[4] = {"up", "down", "left", "right"};
        for (int i = 0; i < 4; ++i) {
            if (!j.contains(keys[i])) fail(path, std::string("axis2d needs '") + keys[i] + "'");
            *sides[i] = parseFilterAt(j[keys[i]], {}, path + "." + keys[i]);
        }
        return std::make_unique<Axis2D>(std::move(spec));
    }
    if (type == "relative") {
        return std::make_unique<Relative>(
            Relative::Spec{to, tags, requiredString(j, "from", path), optionalString(j, "origin")});
    }
    if (type == "sectors") {
        Sectors::Spec spec{to, tags, requiredString(j, "source", path), optionalString(j, "reference"),
                           optionalStrings(j, "names", path), number<float>(j, "offset", 0.f, path),
                           number<float>(j, "deadzone", 0.f, path)};
        if (spec.names.empty()) fail(path, "sectors needs at least one name in 'names'");
        return std::make_unique<Sectors>(std::move(spec));
    }
    if (type == "flick") {
        Flick::Spec spec{to, tags, requiredString(j, "source", path)};
        spec.minSpeed = number<float>(j, "minSpeed", spec.minSpeed, path);
        spec.window = number<Time>(j, "window", spec.window, path);
        spec.minSpan = number<Time>(j, "minSpan", spec.minSpan, path);
        spec.cooldown = number<Time>(j, "cooldown", spec.cooldown, path);
        return std::make_unique<Flick>(std::move(spec));
    }
    if (type == "threshold") {
        if (!j.contains("from")) fail(path, "threshold needs 'from'");
        Threshold::Spec spec{to, tags, parseFilterAt(j["from"], {}, path + ".from")};
        spec.pressAt = number<float>(j, "pressAt", spec.pressAt, path);
        spec.releaseAt = number<float>(j, "releaseAt", spec.releaseAt, path);
        spec.useMagnitude = j.value("magnitude", false);
        return std::make_unique<Threshold>(std::move(spec));
    }
    fail(path, "unknown processor type '" + type + "' (bind, axis2d, relative, sectors, flick, threshold)");
}

const json& arrayAt(const json& root, const char* key) {
    static const json empty = json::array();
    if (!root.contains(key)) return empty;
    if (!root[key].is_array()) fail(key, "expected an array");
    return root[key];
}

} // namespace

Filter parseFilter(const json& j, std::vector<Phase> defaultPhases) {
    return parseFilterAt(j, std::move(defaultPhases), "");
}

Selector parseSelector(const json& j) { return parseSelectorAt(j, ""); }

ConditionPtr parseCondition(const json& j) { return parseConditionAt(j, ""); }

MuteRule parseMute(const json& j) { return parseMuteAt(j, ""); }

Loaded apply(const json& root, InputEngine& engine) {
    if (!root.is_object()) fail("", "root must be an object");
    Loaded loaded;

    const json& bindings = arrayAt(root, "bindings");
    for (std::size_t i = 0; i < bindings.size(); ++i) {
        const std::string path = "bindings[" + std::to_string(i) + "]";
        const json& b = bindings[i];
        if (!b.is_object() || !b.contains("from")) fail(path, "needs 'to' and 'from'");
        const std::string to = requiredString(b, "to", path);
        engine.addProcessor(optionalString(b, "name").value_or(to),
                            std::make_unique<Bind>(parseFilterAt(b["from"], {}, path + ".from"), to,
                                                   optionalStrings(b, "tags", path)));
    }

    const json& processors = arrayAt(root, "processors");
    for (std::size_t i = 0; i < processors.size(); ++i) {
        const std::string path = "processors[" + std::to_string(i) + "]";
        const json& p = processors[i];
        if (!p.is_object()) fail(path, "expected an object");
        auto processor = parseProcessor(p, path);
        engine.addProcessor(optionalString(p, "name").value_or(requiredString(p, "to", path)),
                            std::move(processor));
    }

    const json& channels = arrayAt(root, "channels");
    for (std::size_t i = 0; i < channels.size(); ++i) {
        const std::string path = "channels[" + std::to_string(i) + "]";
        const json& c = channels[i];
        if (!c.is_object() || !c.contains("when")) fail(path, "needs 'name' and 'when'");
        ChannelSpec spec;
        spec.name = requiredString(c, "name", path);
        spec.tags = optionalStrings(c, "tags", path);
        spec.when = parseConditionAt(c["when"], path + ".when");
        spec.cooldown = number<Time>(c, "cooldown", 0.0, path);
        spec.enabled = c.value("enabled", true);
        spec.emit = c.value("emit", true);
        engine.addChannel(std::move(spec));
    }

    const json& mutes = arrayAt(root, "mutes");
    for (std::size_t i = 0; i < mutes.size(); ++i)
        engine.mute(parseMuteAt(mutes[i], "mutes[" + std::to_string(i) + "]"));

    const json& groups = arrayAt(root, "muteGroups");
    for (std::size_t i = 0; i < groups.size(); ++i) {
        const std::string path = "muteGroups[" + std::to_string(i) + "]";
        MuteGroup group;
        group.rule = parseMuteAt(groups[i], path);
        group.name = requiredString(groups[i], "name", path);
        group.toggle = optionalStrings(groups[i], "toggle", path);
        if (group.rule.label.empty()) group.rule.label = group.name;
        loaded.muteGroups.push_back(std::move(group));
    }
    return loaded;
}

Loaded loadFile(const std::filesystem::path& path, InputEngine& engine) {
    std::ifstream file(path);
    if (!file) fail("", "cannot open " + path.string());
    json root;
    try {
        root = json::parse(file, nullptr, true, /*ignore_comments=*/true);
    } catch (const json::parse_error& e) {
        fail("", path.string() + ": " + e.what());
    }
    return config::apply(root, engine);
}

} // namespace gluttony::input::config
