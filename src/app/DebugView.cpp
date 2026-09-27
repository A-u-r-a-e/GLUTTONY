#include "app/DebugView.hpp"

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/VertexArray.hpp>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <numbers>

namespace gluttony::app {

using namespace gluttony::input;

namespace {

const sf::Color kBackground{18, 18, 24};
const sf::Color kPanel{28, 28, 38};
const sf::Color kDim{110, 110, 125};
const sf::Color kText{220, 220, 230};
const sf::Color kDerived{120, 200, 230};
const sf::Color kChannel{240, 200, 90};
const sf::Color kMuted{150, 80, 80};
const sf::Color kActive{120, 230, 140};

constexpr float kCharWidth = 7.8f; // DejaVu Sans Mono at 13px
constexpr float kLine = 16.f;

std::string fit(std::string s, float width) {
    const auto max = static_cast<std::size_t>(width / kCharWidth);
    if (s.size() > max) s = s.substr(0, max > 1 ? max - 1 : 0) + "~";
    return s;
}

std::string fmt(const char* format, double a, double b = 0.0) {
    char buf[64];
    std::snprintf(buf, sizeof buf, format, a, b);
    return buf;
}

std::string tagList(const std::vector<std::string>& tags) {
    std::string out;
    for (const auto& tag : tags) out += (out.empty() ? "" : " ") + tag;
    return out;
}

sf::Vector2f sf2(Vec2 v) { return {v.x, v.y}; }

void line(sf::RenderTarget& target, Vec2 a, Vec2 b, sf::Color color) {
    sf::VertexArray v(sf::PrimitiveType::Lines, 2);
    v[0] = sf::Vertex{sf2(a), color};
    v[1] = sf::Vertex{sf2(b), color};
    target.draw(v);
}

void panel(sf::RenderTarget& target, float x, float y, float w, float h) {
    sf::RectangleShape rect({w, h});
    rect.setPosition({x, y});
    rect.setFillColor(kPanel);
    target.draw(rect);
}

} // namespace

DebugView::DebugView(const sf::Font& font) : text_(font, "", 13) {}

void DebugView::text(sf::RenderTarget& target, std::string_view s, float x, float y, sf::Color color,
                     unsigned size) {
    text_.setString(std::string(s));
    text_.setCharacterSize(size);
    text_.setFillColor(color);
    text_.setPosition({std::round(x), std::round(y)});
    target.draw(text_);
}

void DebugView::draw(sf::RenderTarget& target, const InputEngine& engine, const DebugInfo& info) {
    target.clear(kBackground);
    drawWorld(target, engine, info);
    drawLog(target, engine, info);
    drawChannels(target, engine, info);
    drawFooter(target, engine, info);
}

void DebugView::drawWorld(sf::RenderTarget& target, const InputEngine& engine, const DebugInfo& info) {
    const InputState& state = engine.state();
    const Vec2 p = info.player;
    const Vec2 aim = state.value("aim");
    const float aimAngle = aim.length() > 0.f ? aim.angleDegrees() : 0.f;

    // Aim line.
    if (aim.length() > 0.f) line(target, p, p + aim, sf::Color{80, 80, 110});

    // Movement sectors, drawn relative to aim.
    const char* moveNames[4] = {"toward", "right", "away", "left"};
    for (int i = 0; i < 4; ++i) {
        const float rad = (aimAngle + 90.f * static_cast<float>(i)) * std::numbers::pi_v<float> / 180.f;
        const Vec2 at = p + Vec2{std::cos(rad), std::sin(rad)} * 60.f;
        const bool on = state.isActive(std::string("dir/move/") + moveNames[i]);
        text(target, moveNames[i], at.x - 3.f * static_cast<float>(std::char_traits<char>::length(moveNames[i])),
             at.y - 8.f, on ? kActive : kDim, 12);
    }

    // Recent flick.
    const double flickAge = info.now - info.lastFlickTime;
    if (info.lastFlick && flickAge < 0.4) {
        const auto alpha = static_cast<std::uint8_t>(255.0 * (1.0 - flickAge / 0.4));
        line(target, p, p + info.lastFlick->normalized() * 110.f, sf::Color{240, 120, 200, alpha});
    }

    // Player.
    sf::CircleShape body(12.f);
    body.setOrigin({12.f, 12.f});
    body.setPosition(sf2(p));
    body.setFillColor(state.isActive("move") ? sf::Color{200, 160, 255} : sf::Color{160, 120, 220});
    target.draw(body);
    if (aim.length() > 0.f) line(target, p, p + aim.normalized() * 22.f, sf::Color::White);
}

void DebugView::drawLog(sf::RenderTarget& target, const InputEngine& engine, const DebugInfo& info) {
    const float x = 8.f, y = 8.f, w = 420.f;
    const float h = static_cast<float>(target.getSize().y) - 130.f;
    panel(target, x, y, w, h);
    text(target, info.quietLog ? "SIGNALS  (F9: show continuous)" : "SIGNALS  (F9: hide continuous)",
         x + 6, y + 4, kText);

    float row = y + 4 + kLine * 1.5f;
    for (auto it = engine.log().rbegin(); it != engine.log().rend() && row < y + h - kLine; ++it) {
        const Signal& s = it->signal;
        if (info.quietLog && (s.phase == Phase::Change || s.id == "mouse/raw")) continue;
        sf::Color color = it->muted ? kMuted : s.hasTag("channel") ? kChannel : s.depth > 0 ? kDerived : kText;
        std::string line = fmt("%7.2f ", s.time) + s.id + " " + std::string(phaseName(s.phase));
        if (s.phase == Phase::Change || s.phase == Phase::Pulse) line += fmt(" (%.1f,%.1f)", s.value.x, s.value.y);
        line += "  " + tagList(s.tags);
        if (it->muted) line = "MUTED " + line;
        text(target, fit(line, w - 12), x + 6, row, color, 13);
        row += kLine;
    }
}

void DebugView::drawChannels(sf::RenderTarget& target, const InputEngine& engine, const DebugInfo& info) {
    const float w = 420.f;
    const float x = static_cast<float>(target.getSize().x) - w - 8.f, y = 8.f;
    const float h = static_cast<float>(target.getSize().y) - 130.f;
    panel(target, x, y, w, h);
    text(target, "CHANNELS", x + 6, y + 4, kText);

    float row = y + 4 + kLine * 1.5f;
    for (const auto& channel : engine.channels()) {
        if (row > y + h - kLine * 2) break;
        const bool muted = engine.isChannelMuted(*channel);
        const bool flash = channel->lastFire() && info.now - *channel->lastFire() < 0.35;
        if (flash) {
            sf::RectangleShape bg({w - 8.f, kLine * 2.f});
            bg.setPosition({x + 4.f, row - 1.f});
            bg.setFillColor(sf::Color{90, 75, 30});
            target.draw(bg);
        }

        // Progress through the condition.
        const float progress = channel->condition().progress();
        if (progress > 0.f) {
            sf::RectangleShape bar({(w - 12.f) * progress, 2.f});
            bar.setPosition({x + 6.f, row + kLine * 2.f - 3.f});
            bar.setFillColor(kActive);
            target.draw(bar);
        }

        const sf::Color color = !channel->enabled() || muted ? kDim : flash ? kChannel : kText;
        std::string head = channel->name() + "  x" + std::to_string(channel->fireCount());
        if (muted) head += "  (muted)";
        if (!channel->enabled()) head += "  (disabled)";
        text(target, fit(head, w - 12), x + 6, row, color, 13);
        text(target, fit(channel->condition().describe(), w - 12), x + 6, row + kLine - 2.f, kDim, 11);
        row += kLine * 2.2f;
    }
}

void DebugView::drawFooter(sf::RenderTarget& target, const InputEngine& engine, const DebugInfo& info) {
    const float x = 8.f, y = static_cast<float>(target.getSize().y) - 116.f;
    const float w = static_cast<float>(target.getSize().x) - 16.f;
    panel(target, x, y, w, 108.f);

    // Held / active signals.
    std::string active;
    for (const auto& [id, record] : engine.state().records())
        if (record.active) active += (active.empty() ? "" : "  ") + id;
    text(target, fit("ACTIVE  " + active, w - 12), x + 6, y + 4, kActive);

    // Mute toggles.
    std::string mutes = "MUTES  ";
    if (info.toggles) {
        for (const auto& toggle : *info.toggles) {
            std::string key = toggle.group.toggle.empty() ? "?" : toggle.group.toggle.front();
            if (key.starts_with("key/")) key = key.substr(4);
            mutes += "[" + key + "] " + toggle.group.name + (toggle.active ? " ON" : "") + "   ";
        }
    }
    text(target, fit(mutes, w - 12), x + 6, y + 4 + kLine, kText);

    // Other active mute rules (from config "mutes" or set by code).
    std::string rules;
    for (const auto& [id, rule] : engine.mutes().rules())
        rules += (rules.empty() ? "" : ", ") + (rule.label.empty() ? "#" + std::to_string(id) : rule.label);
    text(target, fit("ACTIVE MUTE RULES  " + (rules.empty() ? std::string("none") : rules), w - 12), x + 6,
         y + 4 + kLine * 2, kDim);

    text(target, fit("F5 reload data/input.json   " + info.status, w - 12), x + 6, y + 4 + kLine * 3.5f,
         info.status.starts_with("error") ? kMuted : kDim);
    text(target, fit(std::string("time ") + fmt("%.2f", info.now) + "   processors " +
                         std::to_string(engine.processors().size()) + "   channels " +
                         std::to_string(engine.channels().size()),
                     w - 12),
         x + 6, y + 4 + kLine * 4.5f, kDim);
}

} // namespace gluttony::app
