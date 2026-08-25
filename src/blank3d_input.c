#include "blank3d_input.h"

#include <ctype.h>
#include <string.h>

static void b3d_zero_poll(void *user, ihk_u8 *out_bits, size_t out_bytes)
{
    size_t i;
    (void)user;
    if (!out_bits) return;
    for (i = 0U; i < out_bytes; ++i) out_bits[i] = 0U;
}

static void b3d_make_zero_backend(ihk_backend *backend)
{
    if (!backend) return;
    backend->user = 0;
    backend->poll_keyboard = b3d_zero_poll;
    backend->shutdown = 0;
    backend->capabilities = IHK_CAP_KEYBOARD;
}

static void b3d_compact_name(const char *source, char *out, int capacity)
{
    int i;
    int j;
    unsigned char c;
    if (!out || capacity <= 0) return;
    if (!source) source = "";
    j = 0;
    for (i = 0; source[i] != '\0' && j + 1 < capacity; ++i) {
        c = (unsigned char)source[i];
        if (isalnum(c)) out[j++] = (char)tolower(c);
    }
    out[j] = '\0';
}

input_key89 blank3d_input_key_from_name(const char *name)
{
    return input_keys89_from_name(name);
}

const char *blank3d_input_key_name(input_key89 key,
                                   char *tmp,
                                   unsigned int tmp_size)
{
    return input_keys89_name(key, tmp, tmp_size);
}

static int b3d_mouse_index(const char *name)
{
    char compact[48];
    b3d_compact_name(name, compact, (int)sizeof(compact));
    if (strcmp(compact, "mouseleft") == 0 ||
        strcmp(compact, "leftmouse") == 0 ||
        strcmp(compact, "mouse1") == 0 ||
        strcmp(compact, "lmb") == 0) return 0;
    if (strcmp(compact, "mouseright") == 0 ||
        strcmp(compact, "rightmouse") == 0 ||
        strcmp(compact, "mouse2") == 0 ||
        strcmp(compact, "rmb") == 0) return 1;
    if (strcmp(compact, "mousemiddle") == 0 ||
        strcmp(compact, "middlemouse") == 0 ||
        strcmp(compact, "mouse3") == 0 ||
        strcmp(compact, "mmb") == 0) return 2;
    if (strcmp(compact, "mousex1") == 0 ||
        strcmp(compact, "mouse4") == 0) return 3;
    if (strcmp(compact, "mousex2") == 0 ||
        strcmp(compact, "mouse5") == 0) return 4;
    return -1;
}

static int b3d_key_bank_query(const Blank3DInput *input,
                              Blank3DInputState state,
                              int usage)
{
    int bank;
    int bit;
    const InputScanner *scanner;
    if (!input || usage < 0 || usage >= 256) return 0;
    bank = usage / B3D_INPUT_BANK_WIDTH;
    bit = usage % B3D_INPUT_BANK_WIDTH;
    if (bank < 0 || bank >= B3D_INPUT_KEY_BANKS) return 0;
    scanner = &input->key_banks[bank];
    if (state == B3D_INPUT_HOLD) return input_button_hold(scanner, bit);
    if (state == B3D_INPUT_PRESSED) return input_button_pressed(scanner, bit);
    if (state == B3D_INPUT_RELEASED) return input_button_released(scanner, bit);
    if (state == B3D_INPUT_REPEAT)
        return input_button_repeat(scanner, bit, 24U, 6U);
    if (state == B3D_INPUT_TAPPED)
        return input_button_tapped(scanner, bit, 10U);
    if (state == B3D_INPUT_LONG_HOLD)
        return input_button_long_hold(scanner, bit, 30U);
    return 0;
}

void blank3d_input_init(Blank3DInput *input)
{
    int i;
    if (!input) return;
    memset(input, 0, sizeof(*input));
    b3d_make_zero_backend(&input->backend);
    input_hook_init(&input->keyboard, &input->backend);
    for (i = 0; i < B3D_INPUT_KEY_BANKS; ++i) {
        (void)input_scanner_init(&input->key_banks[i], 0, 0);
        (void)input_scanner_set_button_count(&input->key_banks[i],
                                              B3D_INPUT_BANK_WIDTH);
    }
    scanemu89_init(&input->capture, input->capture_storage,
                   B3D_INPUT_CAPTURE_BINDINGS);
    input->initialized = 1;
}

void blank3d_input_set_backend(Blank3DInput *input,
                               const ihk_backend *backend)
{
    if (!input) return;
    if (!input->initialized) blank3d_input_init(input);
    if (backend) input->backend = *backend;
    else b3d_make_zero_backend(&input->backend);
    input_hook_set_backend(&input->keyboard, &input->backend);
}

void blank3d_input_set_mouse_buttons(Blank3DInput *input,
                                     int left, int right, int middle,
                                     int x1, int x2)
{
    if (!input) return;
    input->mouse_current[0] = left ? 1 : 0;
    input->mouse_current[1] = right ? 1 : 0;
    input->mouse_current[2] = middle ? 1 : 0;
    input->mouse_current[3] = x1 ? 1 : 0;
    input->mouse_current[4] = x2 ? 1 : 0;
}

static void b3d_capture_key_events(Blank3DInput *input)
{
    int usage;
    scanemu_event event;
    if (!input || !scanemu89_is_listening(&input->capture)) return;
    memset(&event, 0, sizeof(event));
    event.token.type = SCANEMU_T_KEY;
    event.token.device_id = 0;
    for (usage = 0; usage < 256; ++usage) {
        if (input_hook_kb_pressed(&input->keyboard, (ihk_u8)usage)) {
            event.kind = SCANEMU_EV_PRESS;
            event.token.code = usage;
            (void)scanemu89_feed(&input->capture, &event);
            if (!scanemu89_is_listening(&input->capture)) return;
        }
        if (input_hook_kb_released(&input->keyboard, (ihk_u8)usage)) {
            event.kind = SCANEMU_EV_RELEASE;
            event.token.code = usage;
            (void)scanemu89_feed(&input->capture, &event);
            if (!scanemu89_is_listening(&input->capture)) return;
        }
    }
}

static void b3d_capture_mouse_events(Blank3DInput *input)
{
    int i;
    scanemu_event event;
    if (!input || !scanemu89_is_listening(&input->capture)) return;
    memset(&event, 0, sizeof(event));
    event.token.type = SCANEMU_T_MOUSE_BUTTON;
    event.token.device_id = 0;
    for (i = 0; i < 5; ++i) {
        if (input->mouse_pressed[i]) {
            event.kind = SCANEMU_EV_PRESS;
            event.token.code = i + 1;
            (void)scanemu89_feed(&input->capture, &event);
            if (!scanemu89_is_listening(&input->capture)) return;
        }
        if (input->mouse_released[i]) {
            event.kind = SCANEMU_EV_RELEASE;
            event.token.code = i + 1;
            (void)scanemu89_feed(&input->capture, &event);
            if (!scanemu89_is_listening(&input->capture)) return;
        }
    }
}

void blank3d_input_update(Blank3DInput *input)
{
    int bank;
    int bit;
    int usage;
    int i;
    input_bits_t bits;
    if (!input) return;
    if (!input->initialized) blank3d_input_init(input);
    input_hook_update(&input->keyboard);
    for (bank = 0; bank < B3D_INPUT_KEY_BANKS; ++bank) {
        bits = (input_bits_t)0;
        for (bit = 0; bit < B3D_INPUT_BANK_WIDTH; ++bit) {
            usage = bank * B3D_INPUT_BANK_WIDTH + bit;
            if (input_hook_kb_down(&input->keyboard, (ihk_u8)usage))
                bits |= ((input_bits_t)1UL << bit);
        }
        (void)input_scanner_update_with_bits(&input->key_banks[bank], bits);
    }
    for (i = 0; i < 5; ++i) {
        input->mouse_pressed[i] = input->mouse_current[i] &&
                                  !input->mouse_previous[i];
        input->mouse_released[i] = !input->mouse_current[i] &&
                                   input->mouse_previous[i];
    }
    b3d_capture_key_events(input);
    b3d_capture_mouse_events(input);
    for (i = 0; i < 5; ++i)
        input->mouse_previous[i] = input->mouse_current[i];
}

void blank3d_input_shutdown(Blank3DInput *input)
{
    if (!input || !input->initialized) return;
    input_hook_shutdown(&input->keyboard);
    input->initialized = 0;
}

static int b3d_input_hid_usage_from_name(const char *control_name)
{
    input_key89 key;
    unsigned int usage;
    if (!control_name || !*control_name) return -1;
    key = input_keys89_from_name(control_name);
    if (!input_keys89_keyboard_usage(key, &usage)) return -1;
    if (usage > 255U) return -1;
    return (int)usage;
}

int blank3d_input_query(const Blank3DInput *input,
                        Blank3DInputState state,
                        const char *control_name)
{
    int mouse;
    int usage;
    if (!input || !control_name) return 0;
    mouse = b3d_mouse_index(control_name);
    if (mouse >= 0) {
        if (state == B3D_INPUT_HOLD) return input->mouse_current[mouse];
        if (state == B3D_INPUT_PRESSED) return input->mouse_pressed[mouse];
        if (state == B3D_INPUT_RELEASED) return input->mouse_released[mouse];
        if (state == B3D_INPUT_REPEAT) return input->mouse_pressed[mouse];
        if (state == B3D_INPUT_TAPPED) return input->mouse_released[mouse];
        if (state == B3D_INPUT_LONG_HOLD) return input->mouse_current[mouse];
        return 0;
    }
    usage = b3d_input_hid_usage_from_name(control_name);
    if (usage < 0) return 0;
    return b3d_key_bank_query(input, state, usage);
}

int blank3d_input_query_name(const Blank3DInput *input,
                             const char *state_name,
                             const char *control_name)
{
    char compact[32];
    b3d_compact_name(state_name, compact, (int)sizeof(compact));
    if (strcmp(compact, "hold") == 0 || strcmp(compact, "down") == 0)
        return blank3d_input_query(input, B3D_INPUT_HOLD, control_name);
    if (strcmp(compact, "pressed") == 0 || strcmp(compact, "press") == 0)
        return blank3d_input_query(input, B3D_INPUT_PRESSED, control_name);
    if (strcmp(compact, "released") == 0 || strcmp(compact, "release") == 0)
        return blank3d_input_query(input, B3D_INPUT_RELEASED, control_name);
    if (strcmp(compact, "repeat") == 0)
        return blank3d_input_query(input, B3D_INPUT_REPEAT, control_name);
    if (strcmp(compact, "tapped") == 0 || strcmp(compact, "tap") == 0)
        return blank3d_input_query(input, B3D_INPUT_TAPPED, control_name);
    if (strcmp(compact, "longhold") == 0)
        return blank3d_input_query(input, B3D_INPUT_LONG_HOLD, control_name);
    return 0;
}

void blank3d_input_begin_capture(Blank3DInput *input,
                                 const char *symbol,
                                 unsigned long listen_flags)
{
    if (!input) return;
    scanemu89_listen(&input->capture, symbol, (scanemu_u32)listen_flags);
}

void blank3d_input_cancel_capture(Blank3DInput *input)
{
    if (!input) return;
    scanemu89_cancel(&input->capture);
}

int blank3d_input_capture_get(const Blank3DInput *input,
                              const char *symbol,
                              scanemu_token *out_token)
{
    if (!input) return 0;
    return scanemu89_get(&input->capture, symbol, out_token) ? 1 : 0;
}
