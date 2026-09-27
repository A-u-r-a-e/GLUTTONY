// Input playground: feeds real keyboard/mouse/gamepad input through the engine and shows
// everything it sees. Edit data/input.json and press F5 to reload.

#include "app/DebugView.hpp"
#include "app/SfmlInput.hpp"
#include "input/Config.hpp"
#include "input/InputEngine.hpp"

#include <SFML/Graphics.hpp>

#include <algorithm>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;
using namespace gluttony;
using namespace gluttony::input;

namespace {

fs::path findDataDir(int argc, char** argv) {
    if (argc > 1) return argv[1];
    for (const fs::path candidate : {fs::path("data"), fs::path("../data")})
        if (fs::exists(candidate / "input.json")) return candidate;
#ifdef GLUTTONY_DATA_DIR
    return GLUTTONY_DATA_DIR;
#else
    return "data";
#endif
}

} // namespace

int main(int argc, char** argv) {
    const fs::path dataDir = findDataDir(argc, argv);

    sf::RenderWindow window(sf::VideoMode({1280, 720}), "Gluttony - input playground");
    window.setKeyRepeatEnabled(false);
    window.setVerticalSyncEnabled(true);

    sf::Font font;
    if (!font.openFromFile(dataDir / "fonts" / "DejaVuSansMono.ttf")) {
        std::cerr << "Could not load font from " << (dataDir / "fonts").string() << "\n";
        return 1;
    }

    InputEngine engine;
    std::vector<app::MuteToggle> toggles;
    app::DebugInfo info;
    info.toggles = &toggles;

    auto reload = [&] {
        engine.clear();
        toggles.clear();
        try {
            for (auto& group : config::loadFile(dataDir / "input.json", engine).muteGroups)
                toggles.push_back({std::move(group), std::nullopt});
            info.status = "loaded " + (dataDir / "input.json").string();
        } catch (const std::exception& e) {
            info.status = std::string("error: ") + e.what();
            std::cerr << e.what() << "\n";
        }
    };
    reload();

    engine.listen(Filter::id("flick", {Phase::Pulse}), [&](const Signal& s) {
        info.lastFlick = s.value;
        info.lastFlickTime = s.time;
    });

    app::DebugView view(font);
    sf::Clock clock;
    Time last = 0.0;
    info.player = {640.f, 360.f};
    bool playerMoved = true;

    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            const Time now = clock.getElapsedTime().asSeconds();
            if (event->is<sf::Event::Closed>()) {
                window.close();
                break;
            }
            if (const auto* resized = event->getIf<sf::Event::Resized>()) {
                window.setView(sf::View(sf::FloatRect({0.f, 0.f}, sf::Vector2f(resized->size))));
                continue;
            }
            if (event->is<sf::Event::FocusLost>()) {
                for (auto& s : app::releaseHeld(engine.state(), now)) engine.feed(std::move(s));
                continue;
            }

            // Playground controls are handled before the engine so they work while muted.
            if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
                const std::string id = "key/" + app::keyName(key->code, key->scancode);
                if (key->code == sf::Keyboard::Key::F5) {
                    reload();
                    playerMoved = true;
                    continue;
                }
                if (key->code == sf::Keyboard::Key::F9) {
                    info.quietLog = !info.quietLog;
                    continue;
                }
                bool toggled = false;
                for (auto& toggle : toggles) {
                    const auto& keys = toggle.group.toggle;
                    if (std::find(keys.begin(), keys.end(), id) == keys.end()) continue;
                    toggled = true;
                    if (toggle.active) {
                        engine.unmute(*toggle.active);
                        toggle.active.reset();
                    } else {
                        toggle.active = engine.mute(toggle.group.rule);
                    }
                }
                if (toggled) continue;
            }

            for (auto& signal : app::translate(*event, now)) engine.feed(std::move(signal));
        }

        const Time now = clock.getElapsedTime().asSeconds();
        const auto dt = static_cast<float>(now - last);
        last = now;

        // Placeholder "game": the player moves with the engine's move vector.
        if (engine.state().isActive("move")) {
            const Vec2 size{static_cast<float>(window.getSize().x), static_cast<float>(window.getSize().y)};
            info.player = info.player + engine.state().value("move") * (260.f * dt);
            info.player.x = std::clamp(info.player.x, 440.f, size.x - 440.f);
            info.player.y = std::clamp(info.player.y, 20.f, size.y - 140.f);
            playerMoved = true;
        }
        if (playerMoved) {
            engine.feed(Signal{"game/player_position", Phase::Change, info.player, now, {"game", "context"}});
            playerMoved = false;
        }

        engine.update(now);

        info.now = now;
        view.draw(window, engine, info);
        window.display();
    }
}
