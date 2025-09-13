#pragma once

#include "mandelbrot_fractal_utils.hpp"
#include "types.hpp"
#include <stdexec/execution.hpp>

// Убираем предварительное объявление, так как MandelbrotSender не шаблонный
struct MandelbrotSender;

template <typename Receiver>
struct MandelbrotOperationState {
    Receiver receiver_;
    mandelbrot::ViewPort viewport_;
    RenderSettings settings_;
    PixelRegion region_;

    friend void tag_invoke(stdexec::start_t, MandelbrotOperationState &self) noexcept {
        try {
            // Вычисляем итерации для заданной области
            PixelMatrix pixel_data(self.region_.end_row - self.region_.start_row,
                                   std::vector<std::uint32_t>(self.region_.end_col - self.region_.start_col));

            for (std::uint32_t y = self.region_.start_row; y < self.region_.end_row; ++y) {
                for (std::uint32_t x = self.region_.start_col; x < self.region_.end_col; ++x) {
                    const auto complex_point =
                        mandelbrot::Pixel2DToComplex(x, y, self.viewport_, self.settings_.width, self.settings_.height);

                    pixel_data[y - self.region_.start_row][x - self.region_.start_col] =
                        mandelbrot::CalculateIterationsForPoint(complex_point, self.settings_.max_iterations,
                                                                self.settings_.escape_radius);
                }
            }

            // Передаем результат
            stdexec::set_value(std::move(self.receiver_), std::move(pixel_data), self.region_);
        } catch (...) {
            stdexec::set_error(std::move(self.receiver_), std::current_exception());
        }
    }
};

struct MandelbrotSender {
    using is_sender = void;

    mandelbrot::ViewPort viewport_;
    RenderSettings settings_;
    PixelRegion region_;

    // Метод get_completion_signatures
    template <typename Env>
    friend auto tag_invoke(stdexec::get_completion_signatures_t, const MandelbrotSender &, Env) noexcept {
        return stdexec::completion_signatures<stdexec::set_value_t(PixelMatrix,
                                                                   PixelRegion),  // Успешное завершение с данными
                                              stdexec::set_error_t(std::exception_ptr),  // Завершение с ошибкой
                                              stdexec::set_stopped_t()                   // Операция остановлена
                                              >{};
    }

    // Метод connect - создает состояние операции
    template <typename Receiver>
    friend auto tag_invoke(stdexec::connect_t, MandelbrotSender self, Receiver receiver) {
        return MandelbrotOperationState<Receiver>{std::move(receiver), self.viewport_, self.settings_, self.region_};
    }
};

[[nodiscard]] inline auto MakeMandelbrotSender(mandelbrot::ViewPort viewport, RenderSettings settings,
                                               PixelRegion region) {
    return MandelbrotSender{viewport, settings, region};
}