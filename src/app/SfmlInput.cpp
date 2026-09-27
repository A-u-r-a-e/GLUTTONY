#include "app/SfmlInput.hpp"

#include <SFML/Window/Joystick.hpp>

#include <array>
#include <utility>

namespace gluttony::app {

using input::Phase;
using input::Signal;
using input::Time;
using input::Vec2;

namespace {

using Key = sf::Keyboard::Key;

// clang-format off
constexpr std::array kKeyNames = std::to_array<std::pair<Key, const char*>>({
    {Key::A, "A"}, {Key::B, "B"}, {Key::C, "C"}, {Key::D, "D"}, {Key::E, "E"}, {Key::F, "F"},
    {Key::G, "G"}, {Key::H, "H"}, {Key::I, "I"}, {Key::J, "J"}, {Key::K, "K"}, {Key::L, "L"},
    {Key::M, "M"}, {Key::N, "N"}, {Key::O, "O"}, {Key::P, "P"}, {Key::Q, "Q"}, {Key::R, "R"},
    {Key::S, "S"}, {Key::T, "T"}, {Key::U, "U"}, {Key::V, "V"}, {Key::W, "W"}, {Key::X, "X"},
    {Key::Y, "Y"}, {Key::Z, "Z"},
    {Key::Num0, "Num0"}, {Key::Num1, "Num1"}, {Key::Num2, "Num2"}, {Key::Num3, "Num3"},
    {Key::Num4, "Num4"}, {Key::Num5, "Num5"}, {Key::Num6, "Num6"}, {Key::Num7, "Num7"},
    {Key::Num8, "Num8"}, {Key::Num9, "Num9"},
    {Key::Escape, "Escape"}, {Key::LControl, "LControl"}, {Key::LShift, "LShift"},
    {Key::LAlt, "LAlt"}, {Key::LSystem, "LSystem"}, {Key::RControl, "RControl"},
    {Key::RShift, "RShift"}, {Key::RAlt, "RAlt"}, {Key::RSystem, "RSystem"}, {Key::Menu, "Menu"},
    {Key::LBracket, "LBracket"}, {Key::RBracket, "RBracket"}, {Key::Semicolon, "Semicolon"},
    {Key::Comma, "Comma"}, {Key::Period, "Period"}, {Key::Apostrophe, "Apostrophe"},
    {Key::Slash, "Slash"}, {Key::Backslash, "Backslash"}, {Key::Grave, "Grave"},
    {Key::Equal, "Equal"}, {Key::Hyphen, "Hyphen"}, {Key::Space, "Space"}, {Key::Enter, "Enter"},
    {Key::Backspace, "Backspace"}, {Key::Tab, "Tab"}, {Key::PageUp, "PageUp"},
    {Key::PageDown, "PageDown"}, {Key::End, "End"}, {Key::Home, "Home"}, {Key::Insert, "Insert"},
    {Key::Delete, "Delete"}, {Key::Add, "Add"}, {Key::Subtract, "Subtract"},
    {Key::Multiply, "Multiply"}, {Key::Divide, "Divide"}, {Key::Left, "Left"},
    {Key::Right, "Right"}, {Key::Up, "Up"}, {Key::Down, "Down"},
    {Key::Numpad0, "Numpad0"}, {Key::Numpad1, "Numpad1"}, {Key::Numpad2, "Numpad2"},
    {Key::Numpad3, "Numpad3"}, {Key::Numpad4, "Numpad4"}, {Key::Numpad5, "Numpad5"},
    {Key::Numpad6, "Numpad6"}, {Key::Numpad7, "Numpad7"}, {Key::Numpad8, "Numpad8"},
    {Key::Numpad9, "Numpad9"},
    {Key::F1, "F1"}, {Key::F2, "F2"}, {Key::F3, "F3"}, {Key::F4, "F4"}, {Key::F5, "F5"},
    {Key::F6, "F6"}, {Key::F7, "F7"}, {Key::F8, "F8"}, {Key::F9, "F9"}, {Key::F10, "F10"},
    {Key::F11, "F11"}, {Key::F12, "F12"}, {Key::F13, "F13"}, {Key::F14, "F14"}, {Key::F15, "F15"},
    {Key::Pause, "Pause"},
});
// clang-format on

const char* axisName(sf::Joystick::Axis axis) {
    switch (axis) {
    case sf::Joystick::Axis::X: return "X";
    case sf::Joystick::Axis::Y: return "Y";
    case sf::Joystick::Axis::Z: return "Z";
    case sf::Joystick::Axis::R: return "R";
    case sf::Joystick::Axis::U: return "U";
    case sf::Joystick::Axis::V: return "V";
    case sf::Joystick::Axis::PovX: return "PovX";
    case sf::Joystick::Axis::PovY: return "PovY";
    }
    return "?";
}

Vec2 toVec(sf::Vector2i v) { return {static_cast<float>(v.x), static_cast<float>(v.y)}; }

Signal make(std::string id, Phase phase, Vec2 value, Time now, std::vector<std::string> tags) {
    return Signal{std::move(id), phase, value, now, std::move(tags), 0};
}

} // namespace

std::string keyName(sf::Keyboard::Key key, sf::Keyboard::Scancode scancode) {
    for (const auto& [k, name] : kKeyNames)
        if (k == key) return name;
    // Keys SFML can't map to a layout key still get a stable-ish name from their scancode.
    return "Scan" + std::to_string(static_cast<int>(scancode));
}

std::string mouseButtonName(sf::Mouse::Button button) {
    switch (button) {
    case sf::Mouse::Button::Left: return "Left";
    case sf::Mouse::Button::Right: return "Right";
    case sf::Mouse::Button::Middle: return "Middle";
    case sf::Mouse::Button::Extra1: return "Extra1";
    case sf::Mouse::Button::Extra2: return "Extra2";
    }
    return "Unknown";
}

std::vector<Signal> translate(const sf::Event& event, Time now) {
    std::vector<Signal> out;
    const std::vector<std::string> keyTags{"raw", "keyboard", "button"};
    const std::vector<std::string> buttonTags{"raw", "mouse", "button"};

    if (const auto* e = event.getIf<sf::Event::KeyPressed>()) {
        out.push_back(make("key/" + keyName(e->code, e->scancode), Phase::Press, {1.f, 0.f}, now, keyTags));
    } else if (const auto* e = event.getIf<sf::Event::KeyReleased>()) {
        out.push_back(make("key/" + keyName(e->code, e->scancode), Phase::Release, {}, now, keyTags));
    } else if (const auto* e = event.getIf<sf::Event::MouseButtonPressed>()) {
        out.push_back(make("mouse/" + mouseButtonName(e->button), Phase::Press, {1.f, 0.f}, now, buttonTags));
    } else if (const auto* e = event.getIf<sf::Event::MouseButtonReleased>()) {
        out.push_back(make("mouse/" + mouseButtonName(e->button), Phase::Release, {}, now, buttonTags));
    } else if (const auto* e = event.getIf<sf::Event::MouseMoved>()) {
        out.push_back(make("mouse/position", Phase::Change, toVec(e->position), now, {"raw", "mouse", "pointer"}));
    } else if (const auto* e = event.getIf<sf::Event::MouseMovedRaw>()) {
        out.push_back(make("mouse/raw", Phase::Pulse, toVec(e->delta), now, {"raw", "mouse", "motion"}));
    } else if (const auto* e = event.getIf<sf::Event::MouseWheelScrolled>()) {
        const bool vertical = e->wheel == sf::Mouse::Wheel::Vertical;
        out.push_back(make(vertical ? "mouse/wheel" : "mouse/hwheel", Phase::Pulse, {e->delta, 0.f}, now,
                           {"raw", "mouse", "wheel"}));
    } else if (const auto* e = event.getIf<sf::Event::JoystickButtonPressed>()) {
        out.push_back(make("pad" + std::to_string(e->joystickId) + "/button" + std::to_string(e->button),
                           Phase::Press, {1.f, 0.f}, now, {"raw", "pad", "button"}));
    } else if (const auto* e = event.getIf<sf::Event::JoystickButtonReleased>()) {
        out.push_back(make("pad" + std::to_string(e->joystickId) + "/button" + std::to_string(e->button),
                           Phase::Release, {}, now, {"raw", "pad", "button"}));
    } else if (const auto* e = event.getIf<sf::Event::JoystickMoved>()) {
        out.push_back(make("pad" + std::to_string(e->joystickId) + "/axis/" + axisName(e->axis), Phase::Change,
                           {e->position / 100.f, 0.f}, now, {"raw", "pad", "axis"}));
    }
    return out;
}

std::vector<Signal> releaseHeld(const input::InputState& state, Time now) {
    std::vector<Signal> out;
    for (const auto& [id, record] : state.records()) {
        if (record.active && record.last.depth == 0 && record.last.hasTag("raw") && record.last.hasTag("button"))
            out.push_back(make(id, Phase::Release, {}, now, record.last.tags));
    }
    return out;
}

} // namespace gluttony::app
