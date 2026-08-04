/* input_hook_backend_polls_keypc.h

   Adapter backend for input_hook that can consume ANY "polls" keyboard backend
   (polls/key_pc + polls/by_system_backend/*) via a small function-pointer API.

   ✅ Purpose
   - input_hook core stays backend-agnostic.
   - This adapter lets you plug "polls" backends into input_hook.

   🧠 Canonical codes
   - input_hook works with USB HID (page 0x07) usages 0..255.
   - polls uses key_pc_code (logical PC keyboard codes).
   - This adapter maps key_pc_code -> HID usage.

   C89-compatible.
*/

#ifndef INPUT_HOOK_BACKEND_POLLS_KEYPC_H
#define INPUT_HOOK_BACKEND_POLLS_KEYPC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h> /* size_t */

#include "input_hook.h"
#include "polls/key_pc.h" /* key_pc_code, KEY_PC_COUNT */

/* ============================================================
   1) Minimal API that describes a polls backend instance
   ============================================================

   Many polls backends follow this pattern:
     - backend_init(backend*)
     - bind_button(backend*, int button_index, key_pc_code key)
     - backend_update(backend*)
     - button_hold(backend*, int button_index)

   This adapter wraps that pattern so input_hook can read a full keyboard.
*/

typedef void (*ihk_polls_init_fn)(void *backend);
typedef int  (*ihk_polls_bind_fn)(void *backend, int button_index, key_pc_code key);
typedef void (*ihk_polls_update_fn)(void *backend);
typedef int  (*ihk_polls_hold_fn)(const void *backend, int button_index);

typedef struct ihk_polls_kb_backend_api {
    size_t backend_size;      /* sizeof(your_polls_backend_struct) */
    int    max_buttons;       /* max logical buttons per instance (e.g. 64) */

    ihk_polls_init_fn   init; /* optional */
    ihk_polls_bind_fn   bind; /* optional (if NULL, assumes button i == key_pc_code i) */
    ihk_polls_update_fn update; /* optional but recommended */
    ihk_polls_hold_fn   hold; /* REQUIRED: read current DOWN state for a logical button */

    ihk_u32 capabilities;     /* IHK_CAP_* flags (optional info) */
} ihk_polls_kb_backend_api;

/* Helper macro to define a static API object for a polls backend type */
#define IHK_POLLS_DEFINE_API(name, backend_type, max_btn, init_fn, bind_fn, update_fn, hold_fn, caps) \
    static const ihk_polls_kb_backend_api name = { \
        (size_t)sizeof(backend_type), (int)(max_btn), \
        (ihk_polls_init_fn)(init_fn), (ihk_polls_bind_fn)(bind_fn), \
        (ihk_polls_update_fn)(update_fn), (ihk_polls_hold_fn)(hold_fn), \
        (ihk_u32)(caps) \
    }

/* ============================================================
   2) Adapter backend state
   ============================================================

   Why "banks"?
   - Some polls backends are limited to INPUT_MAX_BUTTONS (often 64 bits).
   - key_pc_code may exceed that (KEY_PC_COUNT).
   - We solve it by creating N instances (banks), each responsible for a slice
     of key_pc_code values.

   Example: max_buttons=64, KEY_PC_COUNT=80 => bank_count=2
*/

typedef struct ihk_polls_keypc_backend {
    const ihk_polls_kb_backend_api *api;

    /* Storage for bank instances (either user-provided or malloc) */
    void *storage;
    size_t storage_bytes;
    int owns_storage;

    int bank_count;

    /* Precomputed mapping parameters */
    int keys_per_bank;  /* api->max_buttons */
    int key_count;      /* KEY_PC_COUNT */

    /* Exposed as an input_hook backend */
    ihk_backend out_backend;
} ihk_polls_keypc_backend;

/* ============================================================
   3) Public functions
   ============================================================ */

/* How many banks are needed to cover KEY_PC_COUNT keys? */
int ihk_polls_keypc_bank_count(int max_buttons);

/* Bytes needed for the banks storage (bank_count * api->backend_size) */
size_t ihk_polls_keypc_storage_bytes(const ihk_polls_kb_backend_api *api);

/* Initialize using malloc() for the banks storage */
int ihk_polls_keypc_backend_init_alloc(ihk_polls_keypc_backend *b,
                                      const ihk_polls_kb_backend_api *api);

/* Initialize using caller-provided storage */
int ihk_polls_keypc_backend_init_with_storage(ihk_polls_keypc_backend *b,
                                             const ihk_polls_kb_backend_api *api,
                                             void *storage,
                                             size_t storage_bytes);

/* Shutdown adapter; frees storage only if it was malloc()ed by init_alloc */
void ihk_polls_keypc_backend_shutdown(ihk_polls_keypc_backend *b);

/* Returns a ready-to-use ihk_backend for input_hook_init()
   - pointer is owned by ihk_polls_keypc_backend (do not free)
*/
const ihk_backend *ihk_polls_keypc_backend_as_ihk(const ihk_polls_keypc_backend *b);

#ifdef __cplusplus
}
#endif

#endif /* INPUT_HOOK_BACKEND_POLLS_KEYPC_H */
