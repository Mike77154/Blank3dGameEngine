#include "input_hook_backend_sdl2.h"

#if defined(__has_include)
#  if __has_include(<SDL.h>)
#    include <SDL.h>
#  elif __has_include(<SDL2/SDL.h>)
#    include <SDL2/SDL.h>
#  else
#    error "SDL2 headers not found. Install SDL2 dev package and ensure include path."
#  endif
#else
#  include <SDL.h>
#endif

#include <string.h>

/* Bit helpers */
static void ihk_set_usage_bit(ihk_u8 *bits, int usage)
{
    if (usage < 0 || usage > 255) return;
    bits[(unsigned int)usage >> 3] = (ihk_u8)(bits[(unsigned int)usage >> 3] | (ihk_u8)(1u << ((unsigned int)usage & 7u)));
}

static void ihk_sdl2_poll_keyboard(void *user, ihk_u8 *out_kb_bits, size_t out_bytes)
{
    ihk_sdl2_backend *b;
    const Uint8 *state;
    int nkeys;
    int i;

    b = (ihk_sdl2_backend*)user;
    (void)b;

    /* Always clear */
    if (out_bytes > 0) {
        memset(out_kb_bits, 0, out_bytes);
    }

    if ((SDL_WasInit(SDL_INIT_EVENTS) & SDL_INIT_EVENTS) == 0) {
        if (b && b->auto_init) {
            /* Safe-ish: doesn't create a window, just enables event subsystem */
            (void)SDL_InitSubSystem(SDL_INIT_EVENTS);
        } else {
            return;
        }
    }

    if (b == NULL || b->pump_events) {
        SDL_PumpEvents();
    }

    nkeys = 0;
    state = SDL_GetKeyboardState(&nkeys);
    if (!state || nkeys <= 0) return;

    /* SDL scancodes correspond to USB HID usages (keyboard page 0x07).
       We track only 0..255 usages in this core.
    */
    for (i = 0; i < nkeys && i < 256; ++i) {
        if (state[i]) {
            ihk_set_usage_bit(out_kb_bits, i);
        }
    }
}

void ihk_sdl2_backend_init(ihk_sdl2_backend *b)
{
    if (!b) return;
    b->pump_events = 1;
    b->auto_init = 0;
}

void ihk_sdl2_backend_set_pump_events(ihk_sdl2_backend *b, int enable)
{
    if (!b) return;
    b->pump_events = enable ? 1 : 0;
}

void ihk_sdl2_backend_set_auto_init(ihk_sdl2_backend *b, int enable)
{
    if (!b) return;
    b->auto_init = enable ? 1 : 0;
}

void ihk_sdl2_make_backend(ihk_sdl2_backend *b, ihk_backend *out)
{
    if (!out) return;
    out->user = b;
    out->poll_keyboard = ihk_sdl2_poll_keyboard;
    out->shutdown = 0;
    out->capabilities = IHK_CAP_KEYBOARD | IHK_CAP_LAYOUT_INDEPENDENT;
}
