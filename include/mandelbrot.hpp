#pragma once

#include "mandelbrot_renderer.hpp"
#include <chrono>

class CalculateMandelbrotAsyncSender {
public:
    using is_sender = void;

    explicit CalculateMandelbrotAsyncSender(AppState &state, RenderSettings render_settings,
                                            MandelbrotRenderer &renderer)
        : state_(state), render_settings_{render_settings}, renderer_{renderer} {}

    template <typename Env>
    friend auto tag_invoke(stdexec::get_completion_signatures_t, const CalculateMandelbrotAsyncSender &, Env) noexcept {
        return stdexec::completion_signatures<stdexec::set_value_t(RenderResult),
                                              stdexec::set_error_t(std::exception_ptr), stdexec::set_stopped_t()>{};
    }

    template <typename Receiver>
    struct OperationState {
        Receiver receiver_;
        AppState &state_;
        RenderSettings render_settings_;
        MandelbrotRenderer &renderer_;
        std::optional<stdexec::inplace_stop_source> stop_source_;  // Исправлено здесь

        friend void tag_invoke(stdexec::start_t, OperationState &self) noexcept {
            try {
                if (self.state_.need_rerender) {
                    auto start_time = std::chrono::steady_clock::now();

                    // Запускаем рендеринг асинхронно
                    auto render_sender = self.renderer_.template RenderAsync<THREAD_POOL_SIZE>(self.state_.viewport,
                                                                                               self.render_settings_);

                    // Создаем stop_source для возможной остановки
                    self.stop_source_.emplace();

                    // Запускаем асинхронную операцию рендеринга
                    stdexec::start_detached(
                        std::move(render_sender) | stdexec::then([&self, start_time](RenderResult result) {
                            result.render_time = std::chrono::duration_cast<std::chrono::milliseconds>(
                                std::chrono::steady_clock::now() - start_time);
                            self.state_.need_rerender = false;
                            stdexec::set_value(std::move(self.receiver_), std::move(result));
                        }) |
                        stdexec::upon_error([&self](std::exception_ptr ep) {
                            stdexec::set_error(std::move(self.receiver_), std::move(ep));
                        }) |
                        stdexec::upon_stopped([&self]() { stdexec::set_stopped(std::move(self.receiver_)); }));
                } else {
                    // Если перерисовка не нужна, возвращаем пустой результат
                    RenderResult empty_result;
                    empty_result.viewport = self.state_.viewport;
                    empty_result.settings = self.render_settings_;
                    stdexec::set_value(std::move(self.receiver_), std::move(empty_result));
                }
            } catch (...) {
                stdexec::set_error(std::move(self.receiver_), std::current_exception());
            }
        }
    };

    template <typename Receiver>
    friend OperationState<Receiver> tag_invoke(stdexec::connect_t, CalculateMandelbrotAsyncSender self,
                                               Receiver receiver) {
        return OperationState<Receiver>{std::move(receiver), self.state_, self.render_settings_, self.renderer_};
    }

private:
    RenderSettings render_settings_;
    MandelbrotRenderer &renderer_;
    AppState &state_;
};