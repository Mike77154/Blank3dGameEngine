#include "input_hook_backend_polls89.h"

#include <string.h>

static void ihk_polls89_zero(ihk_u8 *bits, size_t bytes)
{
    size_t i;
    if (!bits) return;
    for (i = 0U; i < bytes; ++i) bits[i] = 0U;
}

static void ihk_polls89_set(ihk_u8 *bits, size_t bytes, unsigned int usage)
{
    size_t byte_index;
    ihk_u8 mask;
    if (!bits || usage >= IHK_KB_USAGE_COUNT) return;
    byte_index = (size_t)(usage >> 3);
    if (byte_index >= bytes) return;
    mask = (ihk_u8)(1U << (usage & 7U));
    bits[byte_index] = (ihk_u8)(bits[byte_index] | mask);
}

static void ihk_polls89_poll_keyboard(void *user,
                                      ihk_u8 *out_kb_bits,
                                      size_t out_bytes)
{
    ihk_polls89_backend *adapter;
    unsigned int usage;

    adapter = (ihk_polls89_backend *)user;
    ihk_polls89_zero(out_kb_bits, out_bytes);
    if (!adapter || !adapter->api || !adapter->api->key_down ||
        !adapter->polls_backend) return;

    if (adapter->api->update)
        adapter->api->update(adapter->polls_backend);

    for (usage = 0U; usage < IHK_KB_USAGE_COUNT; ++usage) {
        input_key89 key;
        key = INPUT_KEY89_KB(usage);
        if (adapter->api->key_down(adapter->polls_backend, key))
            ihk_polls89_set(out_kb_bits, out_bytes, usage);
    }
}

int ihk_polls89_backend_init(ihk_polls89_backend *adapter,
                             void *polls_backend,
                             const ihk_polls89_key_api *api)
{
    if (!adapter || !polls_backend || !api || !api->key_down) return -1;
    memset(adapter, 0, sizeof(*adapter));
    adapter->polls_backend = polls_backend;
    adapter->api = api;
    adapter->out_backend.user = adapter;
    adapter->out_backend.poll_keyboard = ihk_polls89_poll_keyboard;
    adapter->out_backend.shutdown = 0;
    adapter->out_backend.capabilities =
        (ihk_u32)(IHK_CAP_KEYBOARD | api->capabilities);
    return 0;
}

void ihk_polls89_backend_shutdown(ihk_polls89_backend *adapter)
{
    if (!adapter) return;
    memset(adapter, 0, sizeof(*adapter));
}

const ihk_backend *ihk_polls89_backend_as_ihk(const ihk_polls89_backend *adapter)
{
    if (!adapter) return (const ihk_backend *)0;
    return &adapter->out_backend;
}
