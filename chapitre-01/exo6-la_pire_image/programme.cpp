#include <chrono>
#include <cstdint>
#include <cstdio>
#include <vector>

constexpr int WIDTH = 1920;
constexpr int HEIGHT = 1080;
constexpr int CHANNELS = 4;
constexpr int FRAME_COUNT = 1000;

void draw(std::vector<uint8_t>& framebuffer, uint8_t value) {
    std::fill(framebuffer.begin(), framebuffer.end(), value);
}

int main() {
    std::vector<uint8_t> framebuffer(WIDTH * HEIGHT * CHANNELS);
    std::vector<double> durations_ms;
    durations_ms.reserve(FRAME_COUNT);

    for (int i = 0; i < FRAME_COUNT; ++i) {
        auto start = std::chrono::high_resolution_clock::now();
        draw(framebuffer, static_cast<uint8_t>(i % 256));
        auto end = std::chrono::high_resolution_clock::now();

        durations_ms.push_back(
            std::chrono::duration<double, std::milli>(end - start).count());
    }

    double worst = 0.0, total = 0.0;
    int over_budget = 0;
    for (double d : durations_ms) {
        if (d > worst) worst = d;
        if (d > 11.0) over_budget++;
        total += d;
    }

    std::printf("Sur %d images (dessin seul) :\n", FRAME_COUNT);
    std::printf("- Image moyenne : %.3f ms\n", total / FRAME_COUNT);
    std::printf("- Image la plus longue : %.3f ms\n", worst);
    std::printf("- Images > 11 ms : %d\n", over_budget);
}