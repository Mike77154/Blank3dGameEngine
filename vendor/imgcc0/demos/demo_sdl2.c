/*
 * Optional SDL2 example.
 * Build manually after `make`, for example:
 *   cc -std=c89 -pedantic-errors -O2 demos/demo_sdl2.c build/libimgcc0.a \
 *      $(pkg-config --cflags --libs sdl2) -lz -Iinclude
 */
#include "imgcc0.h"
#include <SDL.h>
#include <stdio.h>

#define SDL_DEMO_OUTPUT_BYTES (16U * 1024U * 1024U)
#define SDL_DEMO_TEMP_BYTES   (16U * 1024U * 1024U)
#define SDL_DEMO_FILE_BYTES   (8U * 1024U * 1024U)

IMGCC0_DECLARE_BUFFER(g_sdl_output, SDL_DEMO_OUTPUT_BYTES);
IMGCC0_DECLARE_BUFFER(g_sdl_temp, SDL_DEMO_TEMP_BYTES);
IMGCC0_DECLARE_BUFFER(g_sdl_file, SDL_DEMO_FILE_BYTES);

int main(int argc, char **argv)
{
    imgcc0_image img;
    imgcc0_open_options opt;
    SDL_Window *win;
    SDL_Renderer *ren;
    SDL_Texture *tex;
    int running;
    if (argc < 2) {
        fprintf(stderr, "usage: %s <image-file>\n", argv[0]);
        return 2;
    }
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL init failed: %s\n", SDL_GetError());
        return 1;
    }
    imgcc0_image_init(&img);
    imgcc0_open_options_init(&opt);
    opt.output_buffer = IMGCC0_BUFFER_DATA(g_sdl_output);
    opt.output_buffer_size = IMGCC0_BUFFER_SIZE(g_sdl_output);
    opt.temp_buffer = IMGCC0_BUFFER_DATA(g_sdl_temp);
    opt.temp_buffer_size = IMGCC0_BUFFER_SIZE(g_sdl_temp);
    opt.file_buffer = IMGCC0_BUFFER_DATA(g_sdl_file);
    opt.file_buffer_size = IMGCC0_BUFFER_SIZE(g_sdl_file);
    if (imgcc0_open_file(argv[1], &opt, &img) != IMGCC0_OK || !img.ok || img.frame_count == 0U) {
        fprintf(stderr, "imgcc0_open_file failed: %s\n", img.error_message);
        SDL_Quit();
        return 1;
    }
    win = SDL_CreateWindow("imgcc0 SDL2 demo",
                           SDL_WINDOWPOS_CENTERED,
                           SDL_WINDOWPOS_CENTERED,
                           (int)img.width,
                           (int)img.height,
                           0);
    ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING,
                            (int)img.width, (int)img.height);
    if (tex == 0) {
        fprintf(stderr, "SDL_CreateTexture failed: %s\n", SDL_GetError());
        SDL_DestroyRenderer(ren);
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 1;
    }
    running = 1;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = 0;
        }
        SDL_UpdateTexture(tex, 0, img.frames[0].pixels, (int)img.frames[0].stride);
        SDL_RenderClear(ren);
        SDL_RenderCopy(ren, tex, 0, 0);
        SDL_RenderPresent(ren);
    }
    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    imgcc0_image_reset(&img);
    SDL_Quit();
    return 0;
}
