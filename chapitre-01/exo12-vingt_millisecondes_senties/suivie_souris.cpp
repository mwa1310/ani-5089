#include <SDL2/SDL.h>
#include <deque>
#include <cstdio>
#include <algorithm>


struct Sample {
    Uint32 t_ms;
    float x, y;
};

int main(int argc, char** argv) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::fprintf(stderr, "SDL_Init a echoue: %s\n", SDL_GetError());
        return 1;
    }

    const int W = 1000, H = 700;
    SDL_Window* win = SDL_CreateWindow("Retard: 0 ms  (fleches pour regler, Echap pour quitter)",
                                        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                        W, H, SDL_WINDOW_SHOWN);
    SDL_Renderer* ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    std::deque<Sample> history;
    int delay_ms = 0;
    bool running = true;

    auto draw_dot = [&](float x, float y, int r, Uint8 R, Uint8 G, Uint8 B) {
        SDL_SetRenderDrawColor(ren, R, G, B, 255);
        for (int dy = -r; dy <= r; ++dy)
            for (int dx = -r; dx <= r; ++dx)
                if (dx*dx + dy*dy <= r*r)
                    SDL_RenderDrawPoint(ren, (int)x + dx, (int)y + dy);
    };

    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
            if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                    case SDLK_ESCAPE: running = false; break;
                    case SDLK_UP: delay_ms = std::min(200, delay_ms + 5);  break;
                    case SDLK_DOWN: delay_ms = std::max(0,   delay_ms - 5);  break;
                    case SDLK_RIGHT: delay_ms = std::min(200, delay_ms + 20); break;
                    case SDLK_LEFT: delay_ms = std::max(0,   delay_ms - 20); break;
                }
                char title[128];
                std::snprintf(title, sizeof(title),
                    "Retard: %d ms  (fleches pour regler, Echap pour quitter)", delay_ms);
                SDL_SetWindowTitle(win, title);
            }
        }

        int mx, my;
        SDL_GetMouseState(&mx, &my);
        Uint32 now = SDL_GetTicks();
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

        draw_dot((float)mx, (float)my, 3, 255, 255, 255); 
        draw_dot(dx, dy, 12, 220, 40, 40);

        SDL_RenderPresent(ren);
    }

    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}