#include <chrono>
#include <print>
#include <thread>
#include <utility>

#include <SFML/Graphics.hpp>
#include <exec/repeat_effect_until.hpp>
#include <exec/static_thread_pool.hpp>
#include <stdexec/execution.hpp>

#include "mandelbrot.hpp"
#include "mandelbrot_renderer.hpp"
#include "sfml_events_handler.hpp"
#include "sfml_renderer.hpp"

using namespace std::chrono_literals;

class FrameClock {
public:
    FrameClock() { Reset(); }

    void Reset() noexcept { frame_start_ = std::chrono::steady_clock::now(); }
    auto GetFrameTime() const noexcept { return std::chrono::steady_clock::now() - frame_start_; }

private:
    std::chrono::time_point<std::chrono::steady_clock> frame_start_;
};

class WaitForFPS {
public:
    WaitForFPS(FrameClock &frame_clock, int target_fps)
        : frame_clock_(frame_clock), target_frame_time_(std::chrono::milliseconds(1000 / target_fps)) {}

    void operator()() const {
        auto frame_time = frame_clock_.GetFrameTime();
        if (frame_time < target_frame_time_) {
            auto sleep_time = target_frame_time_ - frame_time;
            std::this_thread::sleep_for(sleep_time);
        }
        frame_clock_.Reset();
    }

private:
    FrameClock &frame_clock_;
    std::chrono::milliseconds target_frame_time_;
};

class MandelbrotApp {
private:
    RenderSettings render_settings_{.width = 800, .height = 600, .max_iterations = 100, .escape_radius = 2.0};

    sf::RenderWindow window_;
    sf::Image image_;
    sf::Texture texture_;
    sf::Sprite sprite_;
    MandelbrotRenderer renderer_;
    AppState state_;

public:
    MandelbrotApp()
        : window_{sf::VideoMode{render_settings_.width, render_settings_.height}, "Mandelbrot Fractal"},
          renderer_{THREAD_POOL_SIZE} {

        image_.create(render_settings_.width, render_settings_.height);
        texture_.create(render_settings_.width, render_settings_.height);

        window_.setKeyRepeatEnabled(false);
    }

    void Run() {
        FrameClock frame_clock;
        sf::Clock zoom_clock;

        // Простой и надежный подход без операторов |
        auto pipeline = stdexec::just() | stdexec::then([this, &zoom_clock]() {
                            // Обрабатываем события SFML напрямую, без создания сложных receiver
                            SfmlEventHandler handler{window_, render_settings_, state_, zoom_clock};

                            // Используем sync_wait для синхронного выполнения
                            stdexec::sync_wait(handler);
                        }) |
                        stdexec::then([this]() {
                            // После обработки событий запускаем рендеринг Мандельброта
                            return CalculateMandelbrotAsyncSender{state_, render_settings_, renderer_};
                        }) |
                        stdexec::let_value([this](RenderResult data) {
                            // Рендерим результат в SFML
                            return SFMLRender{std::move(data), image_, texture_, sprite_, window_, render_settings_};
                        }) |
                        stdexec::then([&frame_clock]() {
                            // Ограничиваем FPS до 60 кадров в секунду
                            WaitForFPS{frame_clock, 60}();
                        });

        // Повторяем пайплайн до тех пор, пока не нужно выходить
        auto repeated_pipeline = std::move(pipeline) |
                                 stdexec::then([this]() { return state_.should_exit; }) |  // Проверяем флаг выхода
                                 exec::repeat_effect_until();  // Повторяем пока не вернется true

        // Синхронно ожидаем завершения всего пайплайна
        stdexec::sync_wait(std::move(repeated_pipeline));
    }
};

int main() {
    try {
        MandelbrotApp app;
        app.Run();
    } catch (const std::exception &e) {
        std::println("Error: {}", e.what());
        return 1;
    }
    return 0;
}