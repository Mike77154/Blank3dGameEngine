#ifndef HOWM89_GENWINCONFIGC89_ADAPTER_H
#define HOWM89_GENWINCONFIGC89_ADAPTER_H

#include "howm89.h"
#include "genwinconfigc89.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*howm89_gwc89_window_hook)(void *user, HOWM89_Window *window);

typedef struct howm89_gwc89_adapter {
    HOWM89_Window *window;
    void *hook_user;
    howm89_gwc89_window_hook after_create;
    howm89_gwc89_window_hook before_destroy;
    int last_play_mode;
    int bound;
} howm89_gwc89_adapter;

void howm89_gwc89_adapter_init(howm89_gwc89_adapter *adapter);
void howm89_gwc89_adapter_set_hooks(howm89_gwc89_adapter *adapter,
                                    void *hook_user,
                                    howm89_gwc89_window_hook after_create,
                                    howm89_gwc89_window_hook before_destroy);
void howm89_gwc89_adapter_make_provider(howm89_gwc89_adapter *adapter,
                                        gwc89_provider *out_provider);
HOWM89_Window *howm89_gwc89_adapter_window(howm89_gwc89_adapter *adapter);
void *howm89_gwc89_adapter_native_handle(howm89_gwc89_adapter *adapter);

#ifdef __cplusplus
}
#endif

#endif
