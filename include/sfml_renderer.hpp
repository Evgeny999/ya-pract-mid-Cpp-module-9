#pragma once

#include <SFML/Graphics.hpp>
#include <optional>
#include <print>
#include <stdexec/execution.hpp>

#include "types.hpp"

class SFMLRender {
public:
    template <typename Receiver>
    struct OperationState {
        Receiver receiver_;
        RenderResult render_result_;
        sf::Image &image_;
        sf::Texture &texture_;
        sf::Sprite &sprite_;
        sf::RenderWindow &window_;
        RenderSettings render_settings_;

        friend void tag_invoke(stdexec::start_t, OperationState &self) noexcept {
            try {
                // Обрабатываем результат рендеринга только если есть данные
                if (!self.render_result_.color_data.empty()) {
                    // Заполняем изображение данными (color_data - это вектор векторов)
                    for (int y = 0; y < self.render_settings_.height; ++y) {
                        for (int x = 0; x < self.render_settings_.width; ++x) {
                            const auto &color = self.render_result_.color_data[y][x];  // Двойной индекс!
                            self.image_.setPixel(x, y, sf::Color(color.r, color.g, color.b));
                        }
                    }

                    // Обновляем текстуру
                    self.texture_.update(self.image_);
                    self.sprite_.setTexture(self.texture_, true);

                    // Очищаем окно и рисуем спрайт
                    self.window_.clear();
                    self.window_.draw(self.sprite_);
                    self.window_.display();
                }

                // Завершаем операцию успешно
                stdexec::set_value(std::move(self.receiver_));
            } catch (...) {
                // В случае ошибки передаем исключение
                stdexec::set_error(std::move(self.receiver_), std::current_exception());
            }
        }
    };

    RenderResult render_result_;
    sf::Image &image_;
    sf::Texture &texture_;
    sf::Sprite &sprite_;
    sf::RenderWindow &window_;
    RenderSettings render_settings_;

    SFMLRender(RenderResult render_result, sf::Image &image, sf::Texture &texture, sf::Sprite &sprite,
               sf::RenderWindow &window, RenderSettings render_settings)
        : render_result_(std::move(render_result)), image_{image}, texture_{texture}, sprite_{sprite}, window_{window},
          render_settings_{render_settings} {}

    // Метод для получения сигнатур завершения
    template <typename Env>
    friend auto tag_invoke(stdexec::get_completion_signatures_t, const SFMLRender &, Env) noexcept {
        return stdexec::completion_signatures<stdexec::set_value_t(), stdexec::set_error_t(std::exception_ptr),
                                              stdexec::set_stopped_t()>{};
    }

    // Метод для подключения получателя
    template <typename Receiver>
    friend OperationState<Receiver> tag_invoke(stdexec::connect_t, SFMLRender self, Receiver receiver) {
        return OperationState<Receiver>{
            std::move(receiver), std::move(self.render_result_), self.image_, self.texture_, self.sprite_,
            self.window_,        self.render_settings_};
    }
};