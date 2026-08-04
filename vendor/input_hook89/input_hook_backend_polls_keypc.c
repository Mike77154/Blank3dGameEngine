/* input_hook_backend_polls_keypc.c
   C89-compatible.
*/

#include "input_hook_backend_polls_keypc.h"

#include <string.h> /* memset */
#include <stdlib.h> /* malloc, free */

/* =========================
   Small helpers
   ========================= */

static void ihk_memzero(void *p, size_t n)
{
    if (p && n) {
        memset(p, 0, n);
    }
}

static void *ihk_bank_ptr(const ihk_polls_keypc_backend *b, int bank_index)
{
    /* bank storage is a raw byte array */
    const char *base;
    if (!b || !b->storage || !b->api) return (void*)0;
    base = (const char*)b->storage;
    return (void*)(base + ((size_t)bank_index * b->api->backend_size));
}

/* =========================
   key_pc_code -> HID usage mapping (Keyboard page 0x07)

   We only map the keys that exist in polls/key_pc.h.
   You can extend this as you extend key_pc_code.
   ========================= */

static ihk_u8 ihk_usage_from_key_pc(key_pc_code k)
{
    /* Letters A..Z -> HID 0x04..0x1D */
    if (k >= KEY_PC_A && k <= KEY_PC_Z) {
        return (ihk_u8)(0x04u + (ihk_u8)(k - KEY_PC_A));
    }

    /* Top row digits: HID 1..0 => 0x1E..0x27 */
    if (k >= KEY_PC_1 && k <= KEY_PC_9) {
        return (ihk_u8)(0x1Eu + (ihk_u8)(k - KEY_PC_1));
    }
    if (k == KEY_PC_0) {
        return (ihk_u8)0x27u;
    }

    /* Function keys F1..F12: 0x3A..0x45 */
    if (k >= KEY_PC_F1 && k <= KEY_PC_F12) {
        return (ihk_u8)(0x3Au + (ihk_u8)(k - KEY_PC_F1));
    }

    /* Keypad digits 0..9 and ops */
    if (k == KEY_PC_KP_0) return (ihk_u8)0x62u;
    if (k == KEY_PC_KP_1) return (ihk_u8)0x59u;
    if (k == KEY_PC_KP_2) return (ihk_u8)0x5Au;
    if (k == KEY_PC_KP_3) return (ihk_u8)0x5Bu;
    if (k == KEY_PC_KP_4) return (ihk_u8)0x5Cu;
    if (k == KEY_PC_KP_5) return (ihk_u8)0x5Du;
    if (k == KEY_PC_KP_6) return (ihk_u8)0x5Eu;
    if (k == KEY_PC_KP_7) return (ihk_u8)0x5Fu;
    if (k == KEY_PC_KP_8) return (ihk_u8)0x60u;
    if (k == KEY_PC_KP_9) return (ihk_u8)0x61u;

    switch (k) {
    /* Arrows */
    case KEY_PC_RIGHT: return (ihk_u8)0x4Fu;
    case KEY_PC_LEFT:  return (ihk_u8)0x50u;
    case KEY_PC_DOWN:  return (ihk_u8)0x51u;
    case KEY_PC_UP:    return (ihk_u8)0x52u;

    /* Modifiers */
    case KEY_PC_LCTRL:  return (ihk_u8)0xE0u;
    case KEY_PC_LSHIFT: return (ihk_u8)0xE1u;
    case KEY_PC_LALT:   return (ihk_u8)0xE2u;
    case KEY_PC_RCTRL:  return (ihk_u8)0xE4u;
    case KEY_PC_RSHIFT: return (ihk_u8)0xE5u;
    case KEY_PC_RALT:   return (ihk_u8)0xE6u;

    /* Specials */
    case KEY_PC_ENTER:     return (ihk_u8)0x28u;
    case KEY_PC_ESCAPE:    return (ihk_u8)0x29u;
    case KEY_PC_BACKSPACE: return (ihk_u8)0x2Au;
    case KEY_PC_TAB:       return (ihk_u8)0x2Bu;
    case KEY_PC_SPACE:     return (ihk_u8)0x2Cu;

    /* Keypad ops */
    case KEY_PC_KP_DIV:   return (ihk_u8)0x54u;
    case KEY_PC_KP_MUL:   return (ihk_u8)0x55u;
    case KEY_PC_KP_SUB:   return (ihk_u8)0x56u;
    case KEY_PC_KP_ADD:   return (ihk_u8)0x57u;
    case KEY_PC_KP_ENTER: return (ihk_u8)0x58u;

    default:
        break;
    }

    return (ihk_u8)0u;
}

static void ihk_set_usage_bit(ihk_u8 *bits, size_t bytes, ihk_u8 usage)
{
    size_t byte_index;
    ihk_u8 mask;

    if (!bits) return;
    byte_index = (size_t)(usage >> 3);
    if (byte_index >= bytes) return;

    mask = (ihk_u8)(1u << (usage & 7u));
    bits[byte_index] = (ihk_u8)(bits[byte_index] | mask);
}

/* =========================
   Backend poll function
   ========================= */

static void ihk_polls_keypc_poll_keyboard(void *user, ihk_u8 *out_kb_bits, size_t out_bytes)
{
    ihk_polls_keypc_backend *b = (ihk_polls_keypc_backend*)user;
    int k;

    if (!out_kb_bits || out_bytes == 0) return;

    /* Clear output */
    ihk_memzero(out_kb_bits, out_bytes);

    if (!b || !b->api || !b->storage || !b->api->hold) {
        return;
    }

    /* Update each bank once */
    if (b->api->update) {
        int bi;
        for (bi = 0; bi < b->bank_count; ++bi) {
            void *inst = ihk_bank_ptr(b, bi);
            if (inst) {
                b->api->update(inst);
            }
        }
    }

    /* Read DOWN state for each key_pc_code and map to HID usage */
    for (k = 0; k < b->key_count; ++k) {
        key_pc_code key = (key_pc_code)k;
        ihk_u8 usage = ihk_usage_from_key_pc(key);
        int bank;
        int idx;
        const void *inst;

        if (usage == 0u) {
            continue;
        }

        /* Determine which bank holds this key */
        bank = k / b->keys_per_bank;
        idx  = k - (bank * b->keys_per_bank);

        if (bank < 0 || bank >= b->bank_count) {
            continue;
        }
        if (idx < 0 || idx >= b->keys_per_bank) {
            continue;
        }

        inst = (const void*)ihk_bank_ptr(b, bank);
        if (!inst) {
            continue;
        }

        if (b->api->hold(inst, idx)) {
            ihk_set_usage_bit(out_kb_bits, out_bytes, usage);
        }
    }
}

static void ihk_polls_keypc_shutdown_backend(void *user)
{
    ihk_polls_keypc_backend *b = (ihk_polls_keypc_backend*)user;
    ihk_polls_keypc_backend_shutdown(b);
}

/* =========================
   Public API
   ========================= */

int ihk_polls_keypc_bank_count(int max_buttons)
{
    int mb = max_buttons;
    int kc = (int)KEY_PC_COUNT;

    if (mb <= 0) return 0;
    /* ceil(kc / mb) */
    return (kc + mb - 1) / mb;
}

size_t ihk_polls_keypc_storage_bytes(const ihk_polls_kb_backend_api *api)
{
    int banks;
    if (!api) return (size_t)0;
    if (api->backend_size == 0 || api->max_buttons <= 0) return (size_t)0;

    banks = ihk_polls_keypc_bank_count(api->max_buttons);
    if (banks <= 0) return (size_t)0;

    return (size_t)banks * api->backend_size;
}

static int ihk_polls_keypc_backend_common_init(ihk_polls_keypc_backend *b,
                                              const ihk_polls_kb_backend_api *api,
                                              void *storage,
                                              size_t storage_bytes,
                                              int owns_storage)
{
    int bank;

    if (!b || !api || !storage) return -1;
    if (api->backend_size == 0) return -1;
    if (api->max_buttons <= 0) return -1;
    if (!api->hold) return -1;

    ihk_memzero(b, sizeof(*b));

    b->api = api;
    b->storage = storage;
    b->storage_bytes = storage_bytes;
    b->owns_storage = owns_storage;

    b->keys_per_bank = api->max_buttons;
    b->key_count = (int)KEY_PC_COUNT;
    b->bank_count = ihk_polls_keypc_bank_count(api->max_buttons);

    /* Initialize each bank instance */
    for (bank = 0; bank < b->bank_count; ++bank) {
        void *inst = ihk_bank_ptr(b, bank);
        int i;

        /* ensure deterministic memory for old backends */
        ihk_memzero(inst, api->backend_size);

        if (api->init) {
            api->init(inst);
        }

        /* Bind button i -> key_pc_code (bank*max_buttons + i) */
        if (api->bind) {
            for (i = 0; i < api->max_buttons; ++i) {
                int key_index = (bank * api->max_buttons) + i;
                key_pc_code key = KEY_PC_NONE;

                if (key_index >= 0 && key_index < (int)KEY_PC_COUNT) {
                    key = (key_pc_code)key_index;
                }

                (void)api->bind(inst, i, key);
            }
        }
    }

    /* Prepare ihk_backend object */
    ihk_memzero(&b->out_backend, sizeof(b->out_backend));
    b->out_backend.user = (void*)b;
    b->out_backend.poll_keyboard = ihk_polls_keypc_poll_keyboard;
    b->out_backend.shutdown = ihk_polls_keypc_shutdown_backend;
    b->out_backend.capabilities = (ihk_u32)(IHK_CAP_KEYBOARD | api->capabilities);

    return 0;
}

int ihk_polls_keypc_backend_init_alloc(ihk_polls_keypc_backend *b,
                                      const ihk_polls_kb_backend_api *api)
{
    size_t need;
    void *mem;

    if (!b || !api) return -1;

    need = ihk_polls_keypc_storage_bytes(api);
    if (need == 0) return -1;

    mem = malloc(need);
    if (!mem) return -1;

    return ihk_polls_keypc_backend_common_init(b, api, mem, need, 1);
}

int ihk_polls_keypc_backend_init_with_storage(ihk_polls_keypc_backend *b,
                                             const ihk_polls_kb_backend_api *api,
                                             void *storage,
                                             size_t storage_bytes)
{
    size_t need;

    if (!b || !api || !storage) return -1;

    need = ihk_polls_keypc_storage_bytes(api);
    if (need == 0) return -1;
    if (storage_bytes < need) return -1;

    return ihk_polls_keypc_backend_common_init(b, api, storage, storage_bytes, 0);
}

void ihk_polls_keypc_backend_shutdown(ihk_polls_keypc_backend *b)
{
    if (!b) return;

    if (b->owns_storage && b->storage) {
        free(b->storage);
    }

    /* clear everything */
    ihk_memzero(b, sizeof(*b));
}

const ihk_backend *ihk_polls_keypc_backend_as_ihk(const ihk_polls_keypc_backend *b)
{
    if (!b) return (const ihk_backend*)0;
    return &b->out_backend;
}
