#pragma once

#include <chrono>
#include <exec/static_thread_pool.hpp>
#include <stdexec/execution.hpp>
#include <tuple>
#include <utility>
#include <vector>

#include "mandelbrot_sender.hpp"
#include "types.hpp"

class MandelbrotRenderer {
private:
    exec::static_thread_pool thread_pool_;

    // Вспомогательная функция для преобразования итераций в цвета
    auto ProcessRegionResult(PixelMatrix pixel_data, PixelRegion region, RenderSettings settings) {
        ColorMatrix color_data(region.end_row - region.start_row,
                               std::vector<mandelbrot::RgbColor>(region.end_col - region.start_col));

        for (std::uint32_t y = 0; y < color_data.size(); ++y) {
            for (std::uint32_t x = 0; x < color_data[y].size(); ++x) {
                color_data[y][x] = mandelbrot::IterationsToColor(pixel_data[y][x], settings.max_iterations);
            }
        }

        return std::make_tuple(std::move(color_data), region);
    }

public:
    explicit MandelbrotRenderer(std::uint32_t num_threads = std::thread::hardware_concurrency())
        : thread_pool_{num_threads} {}

    template <size_t N>
    [[nodiscard]] auto RenderAsync(mandelbrot::ViewPort viewport, RenderSettings settings) {
        const std::uint32_t rows_per_task = (settings.height + N - 1) / N;

        // Создаем сендеры для каждой области
        auto create_sender = [&](std::uint32_t task_index) {
            PixelRegion region{task_index * rows_per_task, std::min((task_index + 1) * rows_per_task, settings.height),
                               0, settings.width};

            return stdexec::schedule(thread_pool_.get_scheduler()) | stdexec::then([=]() { return region; }) |
                   stdexec::let_value([=, viewport = viewport](PixelRegion reg) {
                       return MakeMandelbrotSender(viewport, settings, reg);
                   }) |
                   stdexec::let_value([=, this](PixelMatrix pixel_data, PixelRegion reg) {
                       return stdexec::just(ProcessRegionResult(std::move(pixel_data), reg, settings));
                   });
        };

        // Создаем пачку сендеров с использованием fold expression
        return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            // Создаем все сендеры
            auto senders = std::make_tuple(create_sender(Is)...);

            // Объединяем с помощью when_all
            return stdexec::when_all(std::get<Is>(senders)...) |
                   stdexec::let_value([=, viewport = viewport](auto &&...results) {
                       // Создаем RenderResult и заполняем его
                       RenderResult final_result;
                       final_result.viewport = viewport;
                       final_result.settings = settings;

                       // Инициализируем матрицы
                       final_result.pixel_data =
                           PixelMatrix(settings.height, std::vector<std::uint32_t>(settings.width));
                       final_result.color_data =
                           ColorMatrix(settings.height, std::vector<mandelbrot::RgbColor>(settings.width));

                       // Объединяем результаты из всех областей
                       (
                           [&](const auto &result) {
                               const auto &[color_data, region] = result;
                               for (std::uint32_t y = 0; y < color_data.size(); ++y) {
                                   for (std::uint32_t x = 0; x < color_data[y].size(); ++x) {
                                       final_result.color_data[region.start_row + y][region.start_col + x] =
                                           color_data[y][x];
                                       final_result.pixel_data[region.start_row + y][region.start_col + x] =
                                           mandelbrot::CalculateIterationsForPoint(
                                               mandelbrot::Pixel2DToComplex(region.start_col + x, region.start_row + y,
                                                                            viewport, settings.width, settings.height),
                                               settings.max_iterations, settings.escape_radius);
                                   }
                               }
                           }(results),
                           ...);

                       return stdexec::just(std::move(final_result));
                   });
        }(std::make_index_sequence<N>{});
    }
};