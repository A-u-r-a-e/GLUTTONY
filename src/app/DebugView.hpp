#pragma once

#include "input/Config.hpp"
#include "input/InputEngine.hpp"

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Text.hpp>

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace gluttony::app {

struct MuteToggle {
    input::config::MuteGroup group;
    std::optional<input::MuteId> active;
};

/// Everything the debug view shows that isn't in the engine itself.
struct DebugInfo {
    input::Time now = 0.0;
    input::Vec2 player{};
    std::optional<input::Vec2> lastFlick;
    input::Time lastFlickTime = -10.0;
    const std::vector<MuteToggle>* toggles = nullptr;
    std::string status;
    bool quietLog = true;
};

class DebugView {
public:
    explicit DebugView(const sf::Font& font);
    void draw(sf::RenderTarget& target, const input::InputEngine& engine, const DebugInfo& info);

private:
    void text(sf::RenderTarget& target, std::string_view s, float x, float y, sf::Color color,
              unsigned size = 13);
    void drawWorld(sf::RenderTarget& target, const input::InputEngine& engine, const DebugInfo& info);
    void drawLog(sf::RenderTarget& target, const input::InputEngine& engine, const DebugInfo& info);
    void drawChannels(sf::RenderTarget& target, const input::InputEngine& engine, const DebugInfo& info);
    void drawFooter(sf::RenderTarget& target, const input::InputEngine& engine, const DebugInfo& info);

    sf::Text text_;
};

} // namespace gluttony::app
