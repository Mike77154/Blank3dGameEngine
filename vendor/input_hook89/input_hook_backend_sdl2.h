/* input_hook_backend_sdl2.h

   SDL2 backend for input_hook (USB HID-style).
   SDL2 scancodes are already based on USB HID usage IDs (Keyboard page 0x07).

   ✅ Simple, fast, portable.
   ⚠️ Not global-capture (depends on window focus / SDL event pump).

   Build:
     - Link with SDL2
*/

#ifndef INPUT_HOOK_BACKEND_SDL2_H
#define INPUT_HOOK_BACKEND_SDL2_H

#ifdef __cplusplus
extern "C" {
#endif

#include "input_hook.h"

/* Forward-declare SDL types without including SDL headers here (keeps include hygiene).
   The .c includes <SDL.h>.
*/
typedef struct ihk_sdl2_backend {
    int pump_events; /* call SDL_PumpEvents() in poll (default 1) */
    int auto_init;   /* auto-init SDL_INIT_EVENTS if needed (default 0) */
} ihk_sdl2_backend;

void ihk_sdl2_backend_init(ihk_sdl2_backend *b);
void ihk_sdl2_backend_set_pump_events(ihk_sdl2_backend *b, int enable);
void ihk_sdl2_backend_set_auto_init(ihk_sdl2_backend *b, int enable);

/* Fill an ihk_backend wrapper you can pass into input_hook_init/set_backend */
void ihk_sdl2_make_backend(ihk_sdl2_backend *b, ihk_backend *out);

#ifdef __cplusplus
}
#endif

#endif /* INPUT_HOOK_BACKEND_SDL2_H */
