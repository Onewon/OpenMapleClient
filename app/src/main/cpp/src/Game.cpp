//	This file is part of the continued Journey MMORPG client
//	Copyright (C) 2015-2020  Daniel Allendorf, Ryan Payton
//
//	This program is free software: you can redistribute it and/or modify
//	it under the terms of the GNU Affero General Public License as published by
//	the Free Software Foundation, either version 3 of the License, or
//	(at your option) any later version.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU Affero General Public License for more details.
//
//	You should have received a copy of the GNU Affero General Public License
//	along with this program.  If not, see <https://www.gnu.org/licenses/>.
#include "Game.h"
#include <iostream>
#include <android/log.h>


#include "Constants.h"
#include "Gameplay/Stage.h"
#include "IO/UI.h"
#include "IO/Window.h"
#include "Net/PacketProcessor.h"
#include "Timer.h"
#include "Constants.h"
#include "Util/HardwareInfo.h"
#include "Util/NxFiles.h"
#include "ScreenResolution.h"
#include "Singleton.h"
#include "DisplayModes.h"

namespace ms {
Game::Game() {
    HardwareInfo();
    ScreenResolution();
}

Error Game::init(GLFMDisplay *pApp) {
    const auto *activity = static_cast<ANativeActivity *>(glfmGetAndroidActivity(pApp));
    Configuration::get().set_path(std::string(activity->internalDataPath) + "/Settings");
    const auto width = Setting<Width>::get().load();
    const auto height = Setting<Height>::get().load();
    bool valid_mode = false;
    for (const auto &mode : DISPLAY_MODES) valid_mode |= mode.width == width && mode.height == height;
    if (!valid_mode) {
        Setting<Width>::get().save(1366);
        Setting<Height>::get().save(768);
    }
    if (Error error = PacketProcessor::get().init(pApp)) {
        return error;
    }
    __android_log_print(ANDROID_LOG_INFO, "OpenMapleClient", "Session initialized");
    if (Error error = NxFiles::init(pApp)) {
        return error;
    }
    __android_log_print(ANDROID_LOG_INFO, "OpenMapleClient", "NX resources initialized");

    if (Error error = Window::get().init(pApp)) {
        return error;
    }
    __android_log_print(ANDROID_LOG_INFO, "OpenMapleClient", "Window initialized");
    if (Error error = Music::init()) {
        return error;
    }
    __android_log_print(ANDROID_LOG_INFO, "OpenMapleClient", "Music initialized");
    if (Error error = Sound::init()) {
        return error;
    }
    __android_log_print(ANDROID_LOG_INFO, "OpenMapleClient", "Sound initialized");

    return Error::NONE;
}

void Game::update() {
    Window::get().check_events();
    Window::get().update();
    Stage::get().update();
    UI::get().update();
    PacketProcessor::get().process();
    Music::update_context();
}

void Game::draw(float alpha) {
    Window::get().begin();
    Stage::get().draw(alpha);
    UI::get().draw(alpha);
    Window::get().end();
}

bool Game::is_running() {
    return PacketProcessor::get().is_connected() && UI::get().not_quitted()
           && Window::get().not_closed();
    return true;
}

void Game::game_loop() {
    Timer::get().start();

    const int64_t timestep = Constants::TIMESTEP * 1000;
    int64_t accumulator = timestep;

    while (is_running()) {
        int64_t elapsed = Timer::get().stop();

        // Update game with constant timestep as many times as possible.
        for (accumulator += elapsed; accumulator >= timestep;
             accumulator -= timestep) {
            update();
        }
        // Draw the game. Interpolate to account for remaining time.
        float alpha = static_cast<float>(accumulator) / timestep;
        draw(alpha);
    }

    Sound::close();
}

void Game::start(GLFMDisplay *pApp) {
    // Initialize and check for errors
    if (Error error = init(pApp)) {
        auto message = error.get_message();
        auto details = error.get_details();
        bool can_retry = error.can_retry();
        __android_log_print(ANDROID_LOG_ERROR, "OpenMapleClient", "Initialization failed: %.*s (%.*s)",
            static_cast<int>(message.size()), message.data(), static_cast<int>(details.size()), details.data());

        std::cout << "Error: " << message << std::endl;
        std::cout << "Message: " << details << std::endl;

        if (can_retry) {
            std::cout << "Enter 'retry' to try again." << std::endl;

            std::string command;
            std::cin >> command;

            if (command == "retry") {
                start(pApp);
            }
        }
    }
}
}  // namespace ms
