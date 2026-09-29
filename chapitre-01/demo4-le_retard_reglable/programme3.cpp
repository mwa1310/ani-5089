#include <SDL2/SDL.h>
#include <deque>
#include <vector>
#include <random>
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <fstream>

struct Sample { Uint32 t_ms; float x, y; };

int main() {
    std::vector<int> steps;
    for (int d = 0; d <= 200; d += 20) steps.push_back(d);
    for (int i = 0; i < 4; ++i) steps.push_back(0);

    std::random_device rd;
    std::mt19937 rng(rd());
    std::shuffle(steps.begin(), steps.end(), rng);

    const Uint32 STEP_DURATION_MS = 6000;

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::fprintf(stderr, "SDL_Init a echoue: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window* win = SDL_CreateWindow("Suivi de souris",
                                        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                        1000, 700, SDL_WINDOW_SHOWN);
    SDL_Renderer* ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    std::deque<Sample> history;
    std::ofstream log("results.txt", std::ios::app);
    auto t0 = SDL_GetTicks();
    size_t step_idx = 0;
    Uint32 step_start = t0;

    log << "=== Nouvelle session ===\n";
    std::fprintf(stderr, "[experimentateur] palier 1/%zu : %d ms\n", steps.size(), steps[0]);
    log << "Palier 0 (t=0ms) : " << steps[0] << " ms\n";

    auto draw_dot = [&](float x, float y, int r, Uint8 R, Uint8 G, Uint8 B) {
        SDL_SetRenderDrawColor(ren, R, G, B, 255);
        for (int dy = -r; dy <= r; ++dy)
            for (int dx = -r; dx <= r; ++dx)
                if (dx*dx + dy*dy <= r*r)
                    SDL_RenderDrawPoint(ren, (int)x + dx, (int)y + dy);
    };

    bool running = true;
    while (running && step_idx < steps.size()) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_ESCAPE) running = false;
                if (e.key.keysym.sym == SDLK_SPACE) {
                    Uint32 now = SDL_GetTicks();
                    std::fprintf(stderr, "[experimentateur] SIGNALE a %d ms de retard (palier %zu)\n",
                                 steps[step_idx], step_idx + 1);
                    log << "SIGNALE par le sujet : palier " << (step_idx + 1)
                        << " = " << steps[step_idx] << " ms (t=" << (now - t0) << "ms)\n";
                    log.flush();
                }
            }
        }

        Uint32 now = SDL_GetTicks();
        if (now - step_start > STEP_DURATION_MS) {
            step_idx++;
            step_start = now;
            if (step_idx < steps.size()) {
                std::fprintf(stderr, "[experimentateur] palier %zu/%zu : %d ms\n",
                             step_idx + 1, steps.size(), steps[step_idx]);
                log << "Palier " << step_idx << " (t=" << (now - t0) << "ms) : "
                    << steps[step_idx] << " ms\n";
                log.flush();
            }
        }
        if (step_idx >= steps.size()) break;

        int delay_ms = steps[step_idx];
        int mx, my;
        SDL_GetMouseState(&mx, &my);
        history.push_back({now, (float)mx, (float)my});
        while (!history.empty() && now - history.front().t_ms > 300)
            history.pop_front();

        Uint32 target = (now >= (Uint32)delay_ms) ? now - delay_ms : 0;
        float dx = (float)mx, dy = (float)my;
        for (size_t i = 0; i + 1 < history.size(); ++i) {
            if (history[i].t_ms <= target && history[i+1].t_ms >= target) {
                Uint32 span = history[i+1].t_ms - history[i].t_ms;
                float f = span > 0 ? (float)(target - history[i].t_ms) / span : 0.0f;
                dx = history[i].x + (history[i+1].x - history[i].x) * f;
                dy = history[i].y + (history[i+1].y - history[i].y) * f;
                break;
            }
        }

        SDL_SetRenderDrawColor(ren, 20, 20, 25, 255);
        SDL_RenderClear(ren);
        draw_dot(dx, dy, 12, 220, 40, 40);
        SDL_RenderPresent(ren);
    }

    log << "=== Fin de session ===\n\n";
    log.close();

    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}