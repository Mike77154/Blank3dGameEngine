#include "howm89_genwinconfigc89.h"

#include <string.h>

static int howm89_gwc89_is_fullscreen(int play_mode)
{
    return play_mode == GWC89_MODE_FULLSCREEN ||
           play_mode == GWC89_MODE_BORDERLESS_FULLSCREEN;
}

static int howm89_gwc89_is_decorated(const gwc89_config *config)
{
    if (!config) return 1;
    if (config->play_mode == GWC89_MODE_BORDERLESS ||
        config->play_mode == GWC89_MODE_BORDERLESS_FULLSCREEN)
        return 0;
    return config->decorated ? 1 : 0;
}

static unsigned int howm89_gwc89_flags(const gwc89_config *config)
{
    unsigned int flags;
    flags = 0U;
    if (!config) return flags;
    if (config->visible) flags |= HOWM89_WINDOW_VISIBLE;
    if (config->resizable) flags |= HOWM89_WINDOW_RESIZABLE;
    if (howm89_gwc89_is_decorated(config)) flags |= HOWM89_WINDOW_DECORATED;
    if (config->always_on_top) flags |= HOWM89_WINDOW_TOPMOST;
    if (howm89_gwc89_is_fullscreen(config->play_mode))
        flags |= HOWM89_WINDOW_FULLSCREEN;
    return flags;
}

void howm89_gwc89_adapter_init(howm89_gwc89_adapter *adapter)
{
    if (!adapter) return;
    memset(adapter, 0, sizeof(*adapter));
    adapter->last_play_mode = GWC89_MODE_WINDOWED;
    adapter->bound = 1;
}

void howm89_gwc89_adapter_set_hooks(howm89_gwc89_adapter *adapter,
                                    void *hook_user,
                                    howm89_gwc89_window_hook after_create,
                                    howm89_gwc89_window_hook before_destroy)
{
    if (!adapter) return;
    adapter->hook_user = hook_user;
    adapter->after_create = after_create;
    adapter->before_destroy = before_destroy;
}

static int howm89_gwc89_create(void *user, const gwc89_config *config)
{
    howm89_gwc89_adapter *adapter;
    HOWM89_WindowDesc desc;
    adapter = (howm89_gwc89_adapter *)user;
    if (!adapter || !config || adapter->window) return 0;
    if (!howm89_backend_is_registered()) return 0;
    howm89_window_desc_defaults(&desc);
    desc.title = config->title;
    desc.width = config->client_width;
    desc.height = config->client_height;
    desc.flags = howm89_gwc89_flags(config);
    if (!config->position_auto) {
        desc.x = config->position_x;
        desc.y = config->position_y;
    }
    adapter->window = howm89_window_create_ex(&desc);
    if (!adapter->window) return 0;
    adapter->last_play_mode = config->play_mode;
    if (config->position_auto && config->center_on_create && config->monitor_index == 0) {
        HOWM89_Result center_result;
        center_result = howm89_window_center_on_monitor(adapter->window, 0);
        if (center_result != HOWM89_OK && center_result != HOWM89_ERROR_NOT_SUPPORTED) {
            howm89_window_destroy(adapter->window);
            adapter->window = 0;
            return 0;
        }
    }
    if (adapter->after_create &&
        !adapter->after_create(adapter->hook_user, adapter->window)) {
        howm89_window_destroy(adapter->window);
        adapter->window = 0;
        return 0;
    }
    return 1;
}

static int howm89_gwc89_apply(void *user, const gwc89_config *config)
{
    howm89_gwc89_adapter *adapter;
    HOWM89_Result r;
    int fullscreen;
    int decorated;
    adapter = (howm89_gwc89_adapter *)user;
    if (!adapter || !adapter->window || !config) return 0;

    fullscreen = howm89_gwc89_is_fullscreen(config->play_mode);
    decorated = howm89_gwc89_is_decorated(config);

    if (howm89_gwc89_is_fullscreen(adapter->last_play_mode) && !fullscreen) {
        r = howm89_window_set_fullscreen(adapter->window, HOWM89_FALSE);
        if (r != HOWM89_OK && r != HOWM89_ERROR_NOT_SUPPORTED) return 0;
    }
    r = howm89_window_set_title(adapter->window, config->title);
    if (r != HOWM89_OK && r != HOWM89_ERROR_NOT_SUPPORTED) return 0;
    r = howm89_window_set_resizable(adapter->window,
                                    config->resizable ? HOWM89_TRUE : HOWM89_FALSE);
    if (r != HOWM89_OK && r != HOWM89_ERROR_NOT_SUPPORTED) return 0;
    r = howm89_window_set_decorated(adapter->window,
                                    decorated ? HOWM89_TRUE : HOWM89_FALSE);
    if (r != HOWM89_OK && r != HOWM89_ERROR_NOT_SUPPORTED) return 0;
    r = howm89_window_set_topmost(adapter->window,
                                  config->always_on_top ? HOWM89_TRUE : HOWM89_FALSE);
    if (r != HOWM89_OK && r != HOWM89_ERROR_NOT_SUPPORTED) return 0;

    if (!fullscreen) {
        r = howm89_window_set_size(adapter->window,
                                   config->client_width,
                                   config->client_height);
        if (r != HOWM89_OK && r != HOWM89_ERROR_NOT_SUPPORTED) return 0;
        if (!config->position_auto) {
            r = howm89_window_set_position(adapter->window,
                                           config->position_x,
                                           config->position_y);
            if (r != HOWM89_OK && r != HOWM89_ERROR_NOT_SUPPORTED) return 0;
        }
    }

    if (fullscreen) {
        r = howm89_window_set_fullscreen(adapter->window, HOWM89_TRUE);
        if (r != HOWM89_OK && r != HOWM89_ERROR_NOT_SUPPORTED) return 0;
    }

    r = config->visible ? howm89_window_show(adapter->window)
                        : howm89_window_hide(adapter->window);
    if (r != HOWM89_OK && r != HOWM89_ERROR_NOT_SUPPORTED) return 0;
    adapter->last_play_mode = config->play_mode;
    return 1;
}

static int howm89_gwc89_destroy(void *user)
{
    howm89_gwc89_adapter *adapter;
    adapter = (howm89_gwc89_adapter *)user;
    if (!adapter) return 0;
    if (!adapter->window) return 1;
    if (adapter->before_destroy &&
        !adapter->before_destroy(adapter->hook_user, adapter->window))
        return 0;
    howm89_window_destroy(adapter->window);
    adapter->window = 0;
    adapter->last_play_mode = GWC89_MODE_WINDOWED;
    return 1;
}

static int howm89_gwc89_query(void *user, int *out_width, int *out_height)
{
    howm89_gwc89_adapter *adapter;
    HOWM89_Result r;
    adapter = (howm89_gwc89_adapter *)user;
    if (!adapter || !adapter->window || !out_width || !out_height) return 0;
    r = howm89_window_get_size(adapter->window, out_width, out_height);
    return r == HOWM89_OK && *out_width > 0 && *out_height > 0;
}

void howm89_gwc89_adapter_make_provider(howm89_gwc89_adapter *adapter,
                                        gwc89_provider *out_provider)
{
    if (!out_provider) return;
    memset(out_provider, 0, sizeof(*out_provider));
    if (!adapter) return;
    out_provider->user = adapter;
    out_provider->create_window = howm89_gwc89_create;
    out_provider->apply_window = howm89_gwc89_apply;
    out_provider->destroy_window = howm89_gwc89_destroy;
    out_provider->query_client_size = howm89_gwc89_query;
}

HOWM89_Window *howm89_gwc89_adapter_window(howm89_gwc89_adapter *adapter)
{
    return adapter ? adapter->window : 0;
}

void *howm89_gwc89_adapter_native_handle(howm89_gwc89_adapter *adapter)
{
    if (!adapter || !adapter->window) return 0;
    return howm89_window_get_native_handle(adapter->window);
}
