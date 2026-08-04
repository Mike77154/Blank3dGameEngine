/* input_hook_backend_allegro5.h

   Allegro 5 backend for input_hook (USB HID-style).

   ✅ Works great in Allegro apps.
   ⚠️ Not global-capture; requires Allegro to be initialized and polling is within your process.

   Build:
     - Link with Allegro 5 libraries
*/

#ifndef INPUT_HOOK_BACKEND_ALLEGRO5_H
#define INPUT_HOOK_BACKEND_ALLEGRO5_H

#ifdef __cplusplus
extern "C" {
#endif

#include "input_hook.h"

typedef struct ihk_allegro5_backend {
    int auto_install; /* call al_install_keyboard() if needed (default 0) */
} ihk_allegro5_backend;

void ihk_allegro5_backend_init(ihk_allegro5_backend *b);
void ihk_allegro5_backend_set_auto_install(ihk_allegro5_backend *b, int enable);

void ihk_allegro5_make_backend(ihk_allegro5_backend *b, ihk_backend *out);

#ifdef __cplusplus
}
#endif

#endif /* INPUT_HOOK_BACKEND_ALLEGRO5_H */
