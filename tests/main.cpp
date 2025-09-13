#include "mandelbrot_fractal_utils.hpp"
#include "types.hpp"
#include <SFML/Graphics.hpp>
#include <gtest/gtest.h>

// Тест для проверки вычисления итераций Мандельброта
TEST(MandelbrotTest, CalculateIterationsForPoint) {
    using namespace mandelbrot;

    // Точка внутри множества Мандельброта
    Complex inside_point(-0.5, 0.0);
    uint32_t iterations = CalculateIterationsForPoint(inside_point, 100, 2.0);
    EXPECT_EQ(iterations, 100);  // Должна достичь максимума

    // Точка вне множества Мандельброта
    Complex outside_point(1.0, 1.0);
    iterations = CalculateIterationsForPoint(outside_point, 100, 2.0);
    EXPECT_LT(iterations, 100);  // Должна выйти раньше
}

// Тест для преобразования координат
TEST(MandelbrotTest, Pixel2DToComplex) {
    using namespace mandelbrot;

    ViewPort viewport{-2.0, 2.0, -2.0, 2.0};
    uint32_t screen_width = 800;
    uint32_t screen_height = 600;

    // Центр экрана должен соответствовать (0,0)
    Complex center = Pixel2DToComplex(400, 300, viewport, screen_width, screen_height);
    EXPECT_NEAR(center.real(), 0.0, 0.001);
    EXPECT_NEAR(center.imag(), 0.0, 0.001);

    // Левый верхний угол
    Complex top_left = Pixel2DToComplex(0, 0, viewport, screen_width, screen_height);
    EXPECT_NEAR(top_left.real(), -2.0, 0.001);
    EXPECT_NEAR(top_left.imag(), -2.0, 0.001);
}

// Тест для преобразования итераций в цвет
TEST(MandelbrotTest, IterationsToColor) {
    using namespace mandelbrot;

    // Максимальное количество итераций (черный цвет)
    RgbColor black = IterationsToColor(100, 100);
    EXPECT_EQ(black.r, 0);
    EXPECT_EQ(black.g, 0);
    EXPECT_EQ(black.b, 0);

    // Промежуточное количество итераций (не черный)
    RgbColor color = IterationsToColor(50, 100);
    EXPECT_TRUE(color.r > 0 || color.g > 0 || color.b > 0);
}

// Тест структуры ViewPort
TEST(ViewPortTest, BasicOperations) {
    using namespace mandelbrot;

    ViewPort viewport{-2.5, 1.5, -2.0, 2.0};

    EXPECT_DOUBLE_EQ(viewport.width(), 4.0);   // 1.5 - (-2.5) = 4.0
    EXPECT_DOUBLE_EQ(viewport.height(), 4.0);  // 2.0 - (-2.0) = 4.0
}

// Тест структуры RenderSettings
TEST(RenderSettingsTest, DefaultValues) {
    RenderSettings settings;

    EXPECT_EQ(settings.width, 800);
    EXPECT_EQ(settings.height, 600);
    EXPECT_EQ(settings.max_iterations, 100);
    EXPECT_DOUBLE_EQ(settings.escape_radius, 2.0);
}

// Тест структуры AppState
TEST(AppStateTest, DefaultValues) {
    AppState state;

    EXPECT_FALSE(state.need_rerender);
    EXPECT_FALSE(state.left_mouse_pressed);
    EXPECT_FALSE(state.right_mouse_pressed);
    EXPECT_FALSE(state.should_exit);

    // Проверяем дефолтные значения viewport
    EXPECT_DOUBLE_EQ(state.viewport.x_min, -2.5);
    EXPECT_DOUBLE_EQ(state.viewport.x_max, 1.5);
    EXPECT_DOUBLE_EQ(state.viewport.y_min, -2.0);
    EXPECT_DOUBLE_EQ(state.viewport.y_max, 2.0);
}

// Тест структуры PixelRegion
TEST(PixelRegionTest, BasicOperations) {
    PixelRegion region{0, 100, 0, 200};

    EXPECT_EQ(region.start_row, 0);
    EXPECT_EQ(region.end_row, 100);
    EXPECT_EQ(region.start_col, 0);
    EXPECT_EQ(region.end_col, 200);
}

// Простой тест для RgbColor
TEST(RgbColorTest, BasicOperations) {
    mandelbrot::RgbColor color{255, 128, 64};

    EXPECT_EQ(color.r, 255);
    EXPECT_EQ(color.g, 128);
    EXPECT_EQ(color.b, 64);
}

// Тест для проверки создания MandelbrotRenderer
TEST(MandelbrotRendererTest, Creation) {
    MandelbrotRenderer renderer{4};  // 4 потока

    // Просто проверяем, что объект создается без ошибок
    SUCCEED();
}

// Тест для проверки констант
TEST(ConstantsTest, Values) { EXPECT_EQ(THREAD_POOL_SIZE, 8); }

// Тест для проверки работы FrameClock
TEST(FrameClockTest, BasicOperations) {
    FrameClock clock;

    // Проверяем, что время увеличивается
    auto initial_time = clock.GetFrameTime();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    auto later_time = clock.GetFrameTime();

    EXPECT_GT(later_time, initial_time);

    // Проверяем сброс
    clock.Reset();
    auto reset_time = clock.GetFrameTime();
    EXPECT_LT(reset_time, later_time);
}

// Тест для WaitForFPS
TEST(WaitForFPSTest, BasicOperations) {
    FrameClock clock;
    WaitForFPS wait{clock, 60};

    // Просто проверяем, что объект создается и может быть вызван
    wait();
    SUCCEED();
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
