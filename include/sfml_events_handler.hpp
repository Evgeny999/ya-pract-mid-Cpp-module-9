#pragma once

#include <SFML/Graphics.hpp>
#include <stdexec/execution.hpp>

#include "types.hpp"

class SfmlEventHandler {
public:
    using is_sender = void;

    template <typename Receiver>
    struct OperationState {
        Receiver receiver_;
        sf::RenderWindow &window_;
        RenderSettings render_settings_;
        AppState &state_;
        sf::Clock &zoom_clock_;

        static constexpr float ZOOM_INTERVAL_MS = 100.0f;

        template <typename R>
        explicit OperationState(R &&r, sf::RenderWindow &window, RenderSettings render_settings, AppState &state,
                                sf::Clock &zoom_clock)
            : receiver_{std::forward<R>(r)}, window_{window}, render_settings_{render_settings}, state_{state},
              zoom_clock_{zoom_clock} {}

        friend void tag_invoke(stdexec::start_t, OperationState &self) noexcept {
            try {
                self.HandleEvents();

                if (self.state_.should_exit) {
                    stdexec::set_stopped(std::move(self.receiver_));
                    return;
                }

                self.HandleContinuousZoom();
                stdexec::set_value(std::move(self.receiver_));
            } catch (...) {
                stdexec::set_error(std::move(self.receiver_), std::current_exception());
            }
        }

    private:
        void HandleEvents() {
            sf::Event event;
            while (window_.pollEvent(event)) {
                switch (event.type) {
                case sf::Event::Closed:
                    state_.should_exit = true;
                    break;

                case sf::Event::KeyPressed:
                    if (event.key.code == sf::Keyboard::Escape) {
                        state_.should_exit = true;
                    }
                    break;

                case sf::Event::MouseButtonPressed:
                    if (event.mouseButton.button == sf::Mouse::Left) {
                        state_.left_mouse_pressed = true;
                        sf::Vector2i mouse_pos = sf::Mouse::getPosition(window_);
                        if (mouse_pos.x >= 0 && mouse_pos.x < static_cast<int>(render_settings_.width) &&
                            mouse_pos.y >= 0 && mouse_pos.y < static_cast<int>(render_settings_.height)) {
                            ZoomToPoint(mouse_pos.x, mouse_pos.y, true);
                            state_.need_rerender = true;
                            zoom_clock_.restart();
                        }
                    } else if (event.mouseButton.button == sf::Mouse::Right) {
                        state_.right_mouse_pressed = true;
                        sf::Vector2i mouse_pos = sf::Mouse::getPosition(window_);
                        if (mouse_pos.x >= 0 && mouse_pos.x < static_cast<int>(render_settings_.width) &&
                            mouse_pos.y >= 0 && mouse_pos.y < static_cast<int>(render_settings_.height)) {
                            ZoomToPoint(mouse_pos.x, mouse_pos.y, false);
                            state_.need_rerender = true;
                            zoom_clock_.restart();
                        }
                    }
                    break;

                case sf::Event::MouseButtonReleased:
                    if (event.mouseButton.button == sf::Mouse::Left) {
                        state_.left_mouse_pressed = false;
                    } else if (event.mouseButton.button == sf::Mouse::Right) {
                        state_.right_mouse_pressed = false;
                    }
                    break;

                case sf::Event::MouseWheelScrolled:
                    if (event.mouseWheelScroll.wheel == sf::Mouse::VerticalWheel) {
                        sf::Vector2i mouse_pos(event.mouseWheelScroll.x, event.mouseWheelScroll.y);
                        if (mouse_pos.x >= 0 && mouse_pos.x < static_cast<int>(render_settings_.width) &&
                            mouse_pos.y >= 0 && mouse_pos.y < static_cast<int>(render_settings_.height)) {
                            ZoomToPoint(mouse_pos.x, mouse_pos.y, event.mouseWheelScroll.delta > 0);
                            state_.need_rerender = true;
                        }
                    }
                    break;

                default:
                    break;
                }
            }
        }

        void HandleContinuousZoom() {
            if (zoom_clock_.getElapsedTime().asMilliseconds() < ZOOM_INTERVAL_MS) {
                return;
            }

            if (state_.left_mouse_pressed) {
                sf::Vector2i mouse_pos = sf::Mouse::getPosition(window_);
                if (mouse_pos.x >= 0 && mouse_pos.x < static_cast<int>(render_settings_.width) && mouse_pos.y >= 0 &&
                    mouse_pos.y < static_cast<int>(render_settings_.height)) {
                    ZoomToPoint(mouse_pos.x, mouse_pos.y, true);
                    state_.need_rerender = true;
                    zoom_clock_.restart();
                }
            } else if (state_.right_mouse_pressed) {
                sf::Vector2i mouse_pos = sf::Mouse::getPosition(window_);
                if (mouse_pos.x >= 0 && mouse_pos.x < static_cast<int>(render_settings_.width) && mouse_pos.y >= 0 &&
                    mouse_pos.y < static_cast<int>(render_settings_.height)) {
                    ZoomToPoint(mouse_pos.x, mouse_pos.y, false);
                    state_.need_rerender = true;
                    zoom_clock_.restart();
                }
            }
        }

        void ZoomToPoint(int pixel_x, int pixel_y, bool zoom_in, double factor = 0.8) {
            const double target_x = state_.viewport.x_min +
                                    (static_cast<double>(pixel_x) / render_settings_.width) * state_.viewport.width();
            const double target_y = state_.viewport.y_min +
                                    (static_cast<double>(pixel_y) / render_settings_.height) * state_.viewport.height();

            const double zoom_factor = zoom_in ? factor : (1.0 / factor);
            const double new_width = state_.viewport.width() * zoom_factor;
            const double new_height = state_.viewport.height() * zoom_factor;

            const double new_x_min = target_x - (target_x - state_.viewport.x_min) * zoom_factor;
            const double new_y_min = target_y - (target_y - state_.viewport.y_min) * zoom_factor;

            state_.viewport.x_min = new_x_min;
            state_.viewport.x_max = new_x_min + new_width;
            state_.viewport.y_min = new_y_min;
            state_.viewport.y_max = new_y_min + new_height;
        }
    };

    SfmlEventHandler(sf::RenderWindow &window, RenderSettings render_settings, AppState &state, sf::Clock &zoom_clock)
        : window_{window}, render_settings_{render_settings}, state_{state}, zoom_clock_{zoom_clock} {}

    template <typename Env>
    friend auto tag_invoke(stdexec::get_completion_signatures_t, const SfmlEventHandler &, Env) noexcept {
        return stdexec::completion_signatures<stdexec::set_value_t(), stdexec::set_error_t(std::exception_ptr),
                                              stdexec::set_stopped_t()>{};
    }

    template <typename Receiver>
    friend OperationState<Receiver> tag_invoke(stdexec::connect_t, SfmlEventHandler self, Receiver receiver) {
        return OperationState<Receiver>{std::move(receiver), self.window_, self.render_settings_, self.state_,
                                        self.zoom_clock_};
    }

private:
    sf::RenderWindow &window_;
    RenderSettings render_settings_;
    AppState &state_;
    sf::Clock &zoom_clock_;
};