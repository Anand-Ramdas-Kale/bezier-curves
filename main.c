#include <stdbool.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <SDL.h>

#define HEX_COLOR(hex) \
((hex)>>(3*8)) & 0xFF, \
((hex)>>(2*8)) & 0xFF, \
((hex)>>(1*8)) & 0xFF, \
((hex)>>(0*8)) & 0xFF

#define PS_CAPACITY 256
#define SCREEN_FPS 60
#define DELTA_TIME_SEC (1.0f / SCREEN_FPS)
#define DELTA_TIME_MS (((uint32_t) floorf( DELTA_TIME_SEC * 1000.0f)))
#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080
#define LINE_COLOR 0x444444FF
#define RECT_COLOR 0x87C38FFF
#define MARKER_SIZE 20
#define BLUE_COLOR 0xAAAAFFFF

typedef struct {
    float x, y;
} Vec2;

Vec2 vec2(float x, float y) {return (Vec2) {x, y};}

int check_sdl_code(int code) {
    if (code < 0) {
        fprintf(stderr, "SDL error : %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }
    return code;
}

void *check_sdl_ptr(void *ptr) {
    if (ptr == NULL) {
        fprintf(stderr, "SDL error : %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }
    return ptr;
}

void render_line(SDL_Renderer *renderer, Vec2 begin, Vec2 end, uint32_t color) {
    check_sdl_code(SDL_SetRenderDrawColor(renderer, HEX_COLOR(color)));
    check_sdl_code(SDL_RenderDrawLine(renderer, 
                                      (int) floorf(begin.x), (int) floorf(begin.y), 
                                      (int) floorf(end.x)  , (int) floorf(end.y)));
}

void fill_rect(SDL_Renderer *renderer, Vec2 pos, Vec2 size, uint32_t color) {
    check_sdl_code(SDL_SetRenderDrawColor(renderer, HEX_COLOR(color)));
    SDL_Rect rect = {
        (int) floorf(pos.x),
        (int) floorf(pos.y),
        (int) floorf(size.x),
        (int) floorf(size.y)
    };
    check_sdl_code(SDL_RenderFillRect(renderer, &rect));
}

float lerpf(float a, float b, float p) {
    return a + (b - a) * p;
}

Vec2 vec2_sub(Vec2 a, Vec2 b) {
    return (Vec2){a.x - b.x, a.y - b.y};
}

Vec2 vec2_scale(Vec2 a, float scalar) {
    return (Vec2){scalar * a.x, scalar * a.y};
}

Vec2 vec2_add(Vec2 a, Vec2 b) {
    return (Vec2){a.x + b.x, a.y + b.y};
}

Vec2 lerpv2(Vec2 a, Vec2 b, float p) {
    return vec2_add(a, vec2_scale(vec2_sub(b, a), p));
}

void render_marker(SDL_Renderer *renderer, Vec2 pos, uint32_t color) 
{
    const Vec2 size = vec2(MARKER_SIZE, MARKER_SIZE);
    fill_rect(renderer, vec2_sub(pos, vec2_scale(size, 0.5f)), size, color);
}

Vec2 bezier_sample(Vec2 a, Vec2 b, Vec2 c, Vec2 d, float p)
{
    const Vec2 ab = lerpv2(a, b, p);
    const Vec2 bc = lerpv2(b, c, p);
    const Vec2 cd = lerpv2(c, d, p);
    const Vec2 abc = lerpv2(ab, bc, p);
    const Vec2 bcd = lerpv2(bc, cd, p);
    const Vec2 abcd = lerpv2(abc, bcd, p);
    return abcd;
}

void render_bezier_curve(SDL_Renderer *renderer, Vec2 a, Vec2 b, Vec2 c, Vec2 d, float s, uint32_t color) 
{
    for (float p = 0; p <=1; p += s) {
        Vec2 begin = bezier_sample(a, b, c, d, p);
        Vec2 end   = bezier_sample(a, b, c, d, p + s);
        render_line(renderer, begin, end, color);
    }
}

void render_bezier_marker(SDL_Renderer *renderer, Vec2 a, Vec2 b, Vec2 c, Vec2 d, float s, uint32_t color)
{
    for (float p = 0.0f; p <= 1; p += s) {
       render_marker(renderer, bezier_sample(a, b, c, d, p), color);
    }
}

Vec2 ps[PS_CAPACITY];
size_t ps_count;

int ps_at(Vec2 pos) 
{
    const Vec2 ps_size = vec2(MARKER_SIZE, MARKER_SIZE);
    for (size_t i = 0; i < ps_count; ++i) {
        Vec2 ps_begin = vec2_sub(ps[i], vec2_scale(ps_size, 0.5f));
        Vec2 ps_end   = vec2_add(ps[i], vec2_scale(ps_size, 0.5f));
        if (ps_begin.x <= pos.x && pos.x <= ps_end.x &&
            ps_begin.y <= pos.y && pos.y <= ps_end.y 
        )
        {
            return (int) i;
        }
    }
    return -1;
}

int main(void)
{
    check_sdl_code(SDL_Init(SDL_INIT_VIDEO));
    SDL_SetHint(SDL_HINT_VIDEODRIVER, "wayland");
    SDL_Window * const  window = check_sdl_ptr(SDL_CreateWindow("Bezier Curves",
                                                                0, 0, 
                                                                SCREEN_WIDTH, SCREEN_HEIGHT,
                                                                SDL_WINDOW_RESIZABLE));

    SDL_Renderer *const renderer = check_sdl_ptr(SDL_CreateRenderer(window,
                                                                    -1,
                                                                    SDL_RENDERER_ACCELERATED));

    check_sdl_code(SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH, SCREEN_HEIGHT));

    /*const Vec2 begin = vec2(0.0f, 0.0f);*/
    /*const Vec2 end   = vec2(SCREEN_WIDTH, SCREEN_HEIGHT);*/
    float p = 0.5f;
    (void) p;
    int quit = 0;
    float t = 0.0f;
    int ps_selected = -1;

    while (!quit) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT: {
                    quit = 1;
                    break;
                }
                case SDL_MOUSEBUTTONDOWN: {
                    switch (event.button.button) {
                        case SDL_BUTTON_LEFT: {
                            Vec2 mouse_pos = vec2(event.button.x, event.button.y);
                            if (ps_count < 4) {
                                ps[ps_count++] = mouse_pos;
                            }
                            else {
                                ps_selected = ps_at(mouse_pos);
                            }
                            break;
                        }
                    }
                    break;
                }
                case SDL_MOUSEBUTTONUP: {
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        ps_selected = -1;
                    }
                    break;
                }
                case SDL_MOUSEMOTION: {
                    Vec2 mouse_pos = vec2(event.motion.x, event.motion.y);
                    if (ps_selected != -1) {
                        ps[ps_selected] = mouse_pos;
                    }
                }
            }
        }
        check_sdl_code(SDL_SetRenderDrawColor(renderer, HEX_COLOR(0x000000ff)));
        check_sdl_code(SDL_RenderClear(renderer));
        /*render_line(renderer, vec2(SCREEN_WIDTH ,0.0f), vec2(0.0f, SCREEN_HEIGHT), 0xffffffff);*/
        /*render_line(renderer, vec2(0.0f ,0.0f), vec2(SCREEN_WIDTH, SCREEN_HEIGHT), 0xffffffff);*/
        /*float p = (sinf(t) + 1.0f) * 0.5f;*/
        /*for (size_t i = 0; i < ps_count; ++i) {*/
        /*    render_marker(renderer, ps[i], LINE_COLOR);*/
        /*}*/
        /*for (size_t i = 0; ps_count > 0 && i < ps_count - 1; ++i) {*/
        /*    render_marker(renderer, lerpv2(ps[i], ps[i + 1], p), RECT_COLOR);*/
        /*}*/
        /*for (size_t i = 0; ps_count > 1 && i < ps_count - 2; ++i) {*/
        /*    const Vec2 a = lerpv2(ps[i], ps[i + 1], p);*/
        /*    const Vec2 b = lerpv2(ps[i + 1], ps[i + 2], p);*/
        /*    render_marker(renderer, lerpv2(a, b, p), BLUE_COLOR);*/
        /*}*/
        for (size_t i = 0; i < ps_count; ++i) {
            render_marker(renderer, ps[i], RECT_COLOR);
        }
        if (ps_count >= 4) {
            render_line(renderer, ps[0], ps[1], LINE_COLOR);
            render_line(renderer, ps[2], ps[3], LINE_COLOR);
            render_bezier_curve(renderer, ps[0], ps[1], ps[2], ps[3], 0.01, BLUE_COLOR);
        }
        SDL_RenderPresent(renderer);
        SDL_Delay(DELTA_TIME_MS);
        t += DELTA_TIME_SEC;
    }
    SDL_Quit();
    return EXIT_SUCCESS;
}
