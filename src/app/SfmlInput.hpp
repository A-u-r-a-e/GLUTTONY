#pragma once

#include "input/InputState.hpp"
#include "input/Signal.hpp"

#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

#include <string>
#include <vector>

namespace gluttony::app {

/// Turns one SFML event into engine signals (usually zero or one).
std::vector<input::Signal> translate(const sf::Event& event, input::Time now);

/// Releases for every raw button still held, e.g. when the window loses focus
/// and would otherwise never see the key-up.
std::vector<input::Signal> releaseHeld(const input::InputState& state, input::Time now);

std::string keyName(sf::Keyboard::Key key, sf::Keyboard::Scancode scancode);
std::string mouseButtonName(sf::Mouse::Button button);

} // namespace gluttony::app
