//	This file is part of the continued Journey MMORPG client
//	Copyright (C) 2015-2019  Daniel Allendorf, Ryan Payton
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
#include "Window.h"
#include "../Configuration.h"
#include "../Constants.h"
#include "../Graphics/GraphicsGL.h"
#include "../Timer.h"
#include "UI.h"
#include "stb/stb_image.h"
#include "GLES3/gl32.h"
#include "gl.h"
#include "Char.h"
#include "MapPortals.h"
#include "Singleton.h"
#include "Stage.h"

#include <utility>
#include <algorithm>
#include <cmath>
#include <android/log.h>

namespace ms {
    Window::Window() :
            display(nullptr),
            fullscreen_(true),
            opacity_(1.0f),
            opc_step_(0.0f),
            ratio_x(0.0f),
            ratio_y(0.0f),
            width_(Constants::Constants::Constants::get().get_viewwidth()),
            height_(Constants::Constants::Constants::get().get_viewheight()) {
    }

    Window::~Window() {
        display = nullptr;
//        glfmTerminate();
    }

    void error_callback(int no, const char *description) {
        std::cout << "GLFW error [" << no << "]: " << description << std::endl;
    }

    bool key_callback(GLFMDisplay *display, GLFMKeyCode keyCode, GLFMKeyAction action,
                      int modifiers) {
        std::cout << "key_callback" << keyCode << " action " << action << std::endl;
        UI::get().send_key(keyCode, action != GLFMKeyActionReleased);
        return true;
    }

    void char_callback(GLFMDisplay *display, const char *string, int modifiers) {
        std::cout << "char_callback" << string << std::endl;
        // Determine the length of the wide string (Unicode) to allocate memory
        size_t length = mbstowcs(nullptr, string, 0);
        if (length == (size_t) -1) {
            // Error handling for invalid input string
            return;
        }

        // Allocate memory for the wide string
        wchar_t *wide_string = new wchar_t[length + 1]; // +1 for null terminator

        // Convert the input string to wide string (Unicode)
        mbstowcs(wide_string, string, length);
        wide_string[length] = L'\0'; // Null-terminate the wide string

        // Convert each wide character to uint32_t and use it as needed
        for (size_t i = 0; i < length; ++i) {
            uint32_t unicode_value = static_cast<uint32_t>(wide_string[i]);
            // Now you have the uint32_t representation of the Unicode character
            // You can use it as needed
            UI::get().send_key(unicode_value);
        }
        delete[] wide_string;
    }

    std::chrono::time_point<std::chrono::steady_clock> start =
            ContinuousTimer::get().start();

    std::chrono::time_point<std::chrono::steady_clock> click_start_time;

    bool sent = false;

    bool mousekey_callback(GLFMDisplay *display, int touch, GLFMTouchPhase phase,
                           double x, double y) {
        auto &window = Window::get();
        if (phase == GLFMTouchPhaseCancelled ||
            (!window.contains_touch(x, y) && phase != GLFMTouchPhaseEnded)) {
            if (phase != GLFMTouchPhaseHover) {
                UI::get().send_cursor(false);
                UI::get().release_keys();
                sent = false;
            }
            return true;
        }
        Point<double_t> relative_pos(window.logical_x(x), window.logical_y(y));
        UI::get().set_touch_phase(touch, TouchInfo(relative_pos, phase));
        if (UI::get().should_send_cursor()) {
            Window::get().move_cursor(x, y);
            switch (phase) {
                case GLFMTouchPhaseHover:
                    break;
                case GLFMTouchPhaseBegan: {
                    sent = false;
                    click_start_time = std::chrono::steady_clock::now();
                    UI::get().send_cursor(false);
                    break;
                }
                case GLFMTouchPhaseMoved: {
                    auto diff_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::steady_clock::now() - click_start_time).count();
                    if (diff_ms > 500) {
                        if (!sent) {
                            UI::get().send_cursor(true);
                            sent = true;
                        }
                    } else {
                        UI::get().send_cursor(false);
                    }
                    break;
                }
                case GLFMTouchPhaseEnded: {
                    if (sent)
                        sent = false;
                    else {
                        UI::get().send_cursor(true);
                    }
                    auto diff_ms = ContinuousTimer::get().stop(start) / 1000;
                    start = ContinuousTimer::get().start();

                    if (diff_ms > 10 && diff_ms < 200) {
                        UI::get().doubleclick();
                    }
                    UI::get().send_cursor(false);
                    break;
                }
                case GLFMTouchPhaseCancelled:
                    break;
            }
        }

        return true;
    }

    void Window::move_cursor(double x, double y) {
        auto xpos = static_cast<int16_t>(logical_x(x));
        auto ypos = static_cast<int16_t>(logical_y(y));
        Point<int16_t> pos = Point<int16_t>(xpos, ypos);
        UI::get().send_cursor(pos);
    }

    void focus_callback(GLFMDisplay *display, bool focused) {
        UI::get().send_focus(focused);
    }

    void scroll_callback(GLFMDisplay *display, double xoffset, double yoffset) {
        UI::get().send_scroll(yoffset);
    }

    void close_callback(GLFMDisplay *display) {
        UI::get().send_close();
    }

    void onDraw(GLFMDisplay *display) {
        static bool initialization_failed = false;
        if (initialization_failed) {
            return;
        }
        if (ms::program_ == 0) {
            program_ = glCreateProgram();
            if (Error error = GraphicsGL::get().init(display)) {
                const auto message = error.get_message();
                const auto details = error.get_details();
                __android_log_print(ANDROID_LOG_ERROR, "OpenMapleClient", "Graphics initialization failed: %.*s (%.*s)",
                    static_cast<int>(message.size()), message.data(), static_cast<int>(details.size()), details.data());
                initialization_failed = true;
                return;
            }
            GraphicsGL::get().reinit();
            Char::init();
            DamageNumber::init();
            MapPortals::init();
            Stage::get().init();
            UI::get().init();
            Timer::get().start();

        }
        Window::get().refresh_viewport();
        const int64_t timestep = Constants::TIMESTEP * 1000;
        int64_t accumulator = timestep;
        if (game.is_running()) {
            int64_t elapsed = Timer::get().stop();

            // Update game with constant timestep as many times as possible.
            for (accumulator += elapsed; accumulator >= timestep;
                 accumulator -= timestep) {
                game.update();
            }
            float alpha = static_cast<float>(accumulator) / timestep;
            game.draw(alpha);
        }
    }

    Error Window::init(GLFMDisplay *pApp) {
        display = pApp;
        glfmSetDisplayConfig(display,
                             GLFMRenderingAPIOpenGLES3,
                             GLFMColorFormatRGBA8888,
                             GLFMDepthFormatNone,
                             GLFMStencilFormatNone,
                             GLFMMultisampleNone);
        glfmSetMultitouchEnabled(display, true);
        return init_window();
    }

    Error Window::init_window() {
        glfmSetMouseCursor(display, GLFMMouseCursorDefault);
        glfmSetKeyFunc(display, key_callback);
        glfmSetCharFunc(display, char_callback);
        glfmSetRenderFunc(display, onDraw);
        glfmSetTouchFunc(display, mousekey_callback);
//        glfmSetMouseWheelFunc(display, cursor_callback);
        glfmSetAppFocusFunc(display, focus_callback);
//        glfwSetScrollCallback(pApp, scroll_callback);
        glfmSetSurfaceDestroyedFunc(display, close_callback);
        glfmSetSurfaceResizedFunc(display, [](GLFMDisplay *, int, int) {
            Window::get().refresh_viewport();
        });

        return Error::Code::NONE;
    }

    bool Window::not_closed() const {
//        return glfwWindowShouldClose(glwnd_) == 0;
        return true;
    }

    void Window::update() {
        update_opc();
    }

    void Window::update_opc() {
        if (opc_step_ != 0.0f) {
            opacity_ += opc_step_;

            if (opacity_ >= 1.0f) {
                opacity_ = 1.0f;
                opc_step_ = 0.0f;
            } else if (opacity_ <= 0.0f) {
                opacity_ = 0.0f;
                opc_step_ = -opc_step_;

                fade_procedure_();
            }
        }
    }

    void Window::check_events() {
        int16_t max_width = Configuration::get().get_max_width();
        int16_t max_height = Configuration::get().get_max_height();
        int16_t new_width = Constants::Constants::Constants::get().get_viewwidth();
        int16_t new_height = Constants::Constants::Constants::get().get_viewheight();

        if (width_ != new_width || height_ != new_height) {
            width_ = new_width;
            height_ = new_height;

            if (new_width >= max_width || new_height >= max_height) {
                fullscreen_ = true;
            }

//            init_window();
        }

//        glfwPollEvents();
    }

    void Window::begin() const {
        GraphicsGL::get().clear_scene();
    }

    void Window::end() const {
        GraphicsGL::get().flush(opacity_);
        glfmSwapBuffers(display);
    }

    void Window::fadeout(float step, std::function<void()> fadeproc) {
        opc_step_ = -step;
        fade_procedure_ = std::move(fadeproc);
    }

    void Window::set_clipboard(const std::string &text) const {
//        glfwSetClipboardString(glwnd_, text.c_str());
    }

    std::string Window::get_clipboard() const {
//        const char *text = glfwGetClipboardString(glwnd_);

//        return text != nullptr ? text : "";
        return "";
    }

    void Window::toggle_fullscreen() {
        int16_t max_width = Configuration::get().get_max_width();
        int16_t max_height = Configuration::get().get_max_height();

        if (width_ < max_width && height_ < max_height) {
            fullscreen_ = !fullscreen_;
            Setting<Fullscreen>::get().save(fullscreen_);

//            init_window();
//            glfwPollEvents();
        }
    }

    double Window::get_ratio_x() {
        return ratio_x;
    }

    void Window::refresh_viewport() {
        if (ms::program_ == 0) return;
        int surface_width, surface_height;
        glfmGetDisplaySize(display, &surface_width, &surface_height);
        const int logical_width = Constants::Constants::get().get_viewwidth();
        const int logical_height = Constants::Constants::get().get_viewheight();
        if (surface_width <= 0 || surface_height <= 0 || logical_width <= 0 || logical_height <= 0) return;
        const double scale = std::min(double(surface_width) / logical_width,
                                      double(surface_height) / logical_height);
        const int width = std::max(1, int(std::lround(logical_width * scale)));
        const int height = std::max(1, int(std::lround(logical_height * scale)));
        if (width == viewport_width_ && height == viewport_height_ &&
            surface_width == surface_width_ && surface_height == surface_height_ &&
            ratio_x == double(width) / logical_width && ratio_y == double(height) / logical_height) return;
        surface_width_ = surface_width;
        surface_height_ = surface_height;
        viewport_width_ = width;
        viewport_height_ = height;
        viewport_x_ = (surface_width - width) / 2;
        viewport_y_ = (surface_height - height) / 2;
        set_ratio(double(width) / logical_width, double(height) / logical_height);
        glViewport(viewport_x_, surface_height - viewport_y_ - height, width, height);
        GraphicsGL::get().reinit();
        UI::get().release_keys();
        __android_log_print(ANDROID_LOG_INFO, "DisplayMode", "surface=%dx%d logical=%dx%d viewport=%d,%d %dx%d",
            surface_width, surface_height, logical_width, logical_height, viewport_x_, viewport_y_, width, height);
    }

    double Window::logical_x(double x) const { return ratio_x > 0 ? (x - viewport_x_) / ratio_x : 0; }
    double Window::logical_y(double y) const { return ratio_y > 0 ? (y - viewport_y_) / ratio_y : 0; }
    bool Window::contains_touch(double x, double y) const {
        return x >= viewport_x_ && y >= viewport_y_ &&
               x < viewport_x_ + viewport_width_ && y < viewport_y_ + viewport_height_;
    }

    double Window::get_ratio_y() {
        return ratio_y;
    }

    void Window::set_ratio(double ratio_x, double ratio_y) {
        this->ratio_x = ratio_x;
        this->ratio_y = ratio_y;
    }

    GLFMDisplay *Window::get_display() {
        return display;
    }
}  // namespace ms
