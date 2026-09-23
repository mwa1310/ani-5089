#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <vector>

constexpr int WIDTH = 1920;
constexpr int HEIGHT = 1080;
constexpr int CHANNELS = 4;
constexpr int FRAME_COUNT = 1000;
constexpr int ENTITY_COUNT = 2000;

struct Entity {
    float x, y, z;
    float vx, vy, vz;
};

void update_logic(std::vector<Entity>& entities, float dt) {
    for (auto& e : entities) {
        e.x += e.vx * dt;
        e.y += e.vy * dt;
        e.z += e.vz * dt;
        e.vx = std::cos(e.x * 0.001f);
        e.vy = std::sin(e.y * 0.001f);
    }
}

void draw(std::vector<uint8_t>& framebuffer, float t) {
    for (int y = 0; y < HEIGHT; ++y) {
        for (int x = 0; x < WIDTH; ++x) {
            int idx = (y * WIDTH + x) * CHANNELS;
            uint8_t v = static_cast<uint8_t>(
                128.0f + 127.0f * std::sin((x + t) * 0.01f) * std::cos((y + t) * 0.01f));
            framebuffer[idx + 0] = v;
            framebuffer[idx + 1] = v;
            framebuffer[idx + 2] = v;
            framebuffer[idx + 3] = 255;
        }
    }
}

int main() {
    std::vector<uint8_t> framebuffer(WIDTH * HEIGHT * CHANNELS);
    std::vector<Entity> entities(ENTITY_COUNT);
    for (auto& e : entities) { e.x = e.y = e.z = 0; e.vx = e.vy = e.vz = 0.01f; }

    std::vector<double> render_ms;
    render_ms.reserve(FRAME_COUNT);

    for (int i = 0; i < FRAME_COUNT; ++i) {
        update_logic(entities, 0.016f);

        auto start = std::chrono::high_resolution_clock::now();
        draw(framebuffer, static_cast<float>(i));
        auto end = std::chrono::high_resolution_clock::now();

        render_ms.push_back(std::chrono::duration<double, std::milli>(end - start).count());
    }

    double worst = 0.0, total = 0.0;
    int over_budget = 0;
    constexpr double BUDGET_MONO_MS = 11.1; 
    for (double d : render_ms) {
        if (d > worst) worst = d;
        if (d > BUDGET_MONO_MS) over_budget++;
        total += d;
    }
    double average = total / FRAME_COUNT;
    double worst_stereo = worst * 2.0;
    double average_stereo = average * 2.0;
    double remaining_worst = BUDGET_MONO_MS - worst_stereo;
    double remaining_average = BUDGET_MONO_MS - average_stereo;

    std::printf("Sur %d images (rendu seul, logique exclue) :\n", FRAME_COUNT);
    std::printf("- Rendu moyen (1 oeil) : %.3f ms\n", average);
    std::printf("- Rendu le plus long (1 oeil): %.3f ms\n", worst);
    std::printf("- Images > %.1f ms (1 oeil)  : %d\n", BUDGET_MONO_MS, over_budget);
    std::printf("\nEstimation en stereo (x2) :\n");
    std::printf("- Rendu moyen (2 yeux) : %.3f ms\n", average_stereo);
    std::printf("- Rendu le plus long (2 yeux): %.3f ms\n", worst_stereo);
    std::printf("\nBudget image a 90 Hz : %.1f ms\n", BUDGET_MONO_MS);
    std::printf("Reste apres rendu stereo (cas moyen) : %.3f ms\n", remaining_average);
    std::printf("Reste apres rendu stereo (pire cas)  : %.3f ms\n", remaining_worst);

    return 0;
}