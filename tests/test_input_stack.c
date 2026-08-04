#include "blank3d_input.h"
#include "blank3d_ddsl_input.h"

#include <stdio.h>
#include <string.h>

typedef struct TestKeyboardTag {
    unsigned char bits[IHK_KB_BITS_BYTES];
} TestKeyboard;

static void test_poll(void *user, ihk_u8 *out_bits, size_t out_bytes)
{
    TestKeyboard *keyboard;
    size_t i;
    keyboard = (TestKeyboard *)user;
    if (!keyboard || !out_bits) return;
    for (i = 0U; i < out_bytes && i < IHK_KB_BITS_BYTES; ++i)
        out_bits[i] = keyboard->bits[i];
}

static void set_key(TestKeyboard *keyboard, int usage, int down)
{
    unsigned char mask;
    int byte_index;
    if (!keyboard || usage < 0 || usage >= 256) return;
    byte_index = usage >> 3;
    mask = (unsigned char)(1U << (usage & 7));
    if (down) keyboard->bits[byte_index] |= mask;
    else keyboard->bits[byte_index] &= (unsigned char)(~mask);
}

int main(void)
{
    Blank3DInput input;
    TestKeyboard keyboard;
    ihk_backend backend;
    scanemu_token token;
    Blank3DDdslInputRegistry registry;
    char output[1024];
    char error[160];
    const char *source;

    memset(&keyboard, 0, sizeof(keyboard));
    memset(&backend, 0, sizeof(backend));
    backend.user = &keyboard;
    backend.poll_keyboard = test_poll;
    backend.capabilities = IHK_CAP_KEYBOARD;

    blank3d_input_init(&input);
    blank3d_input_set_backend(&input, &backend);

    if (blank3d_input_key_from_name("W") != KEY_PC_W ||
        blank3d_input_key_from_name("Up") != KEY_PC_UP ||
        blank3d_input_key_from_name("PageUp") != KEY_PC_PAGE_UP ||
        blank3d_input_key_from_name("LeftShift") != KEY_PC_LSHIFT ||
        blank3d_input_key_from_name("F24") != KEY_PC_F24) {
        puts("key name normalization failed");
        return 1;
    }

    set_key(&keyboard, KEY_PC_W, 1);
    set_key(&keyboard, KEY_PC_UP, 1);
    blank3d_input_update(&input);
    if (!blank3d_input_query_name(&input, "hold", "W") ||
        !blank3d_input_query_name(&input, "pressed", "Up")) {
        puts("hold/pressed query failed");
        return 2;
    }

    blank3d_input_update(&input);
    if (blank3d_input_query_name(&input, "pressed", "W") ||
        !blank3d_input_query_name(&input, "hold", "Up")) {
        puts("edge persistence failed");
        return 3;
    }

    set_key(&keyboard, KEY_PC_W, 0);
    blank3d_input_update(&input);
    if (!blank3d_input_query_name(&input, "released", "W")) {
        puts("release query failed");
        return 4;
    }

    blank3d_input_begin_capture(&input, "jump", SCANEMU_LISTEN_KEYS |
                                               SCANEMU_CAPTURE_ON_PRESS);
    set_key(&keyboard, KEY_PC_J, 1);
    blank3d_input_update(&input);
    if (!blank3d_input_capture_get(&input, "jump", &token) ||
        token.type != SCANEMU_T_KEY || token.code != KEY_PC_J) {
        puts("scanemu89 capture failed");
        return 5;
    }

    source = "If key_hold W then move_foward\n"
             "If key_hold Up then move_forward\n"
             "If key_pressed J then jump = 1\n"
             "If key_press M then list_cyclenext=active_weapon\n"
             "if key-mouse-left = 1 then shoot = 1\n";
    if (!blank3d_ddsl_input_preprocess(source, output,
            (unsigned int)sizeof(output), &registry,
            error, (unsigned int)sizeof(error))) {
        printf("preprocess failed: %s\n", error);
        return 6;
    }
    if (registry.count != 5 ||
        strstr(output, "key_hold") != 0 ||
        strstr(output, "key-mouse-left") != 0 ||
        strstr(output, "__input_000") == 0 ||
        strstr(output, "__input_004") == 0 ||
        strstr(output, "move_foward = 1") == 0 ||
        strstr(output, "move_forward = 1") == 0 ||
        strstr(output, "list_cyclenext=active_weapon") == 0) {
        printf("preprocess output invalid:\n%s\n", output);
        return 7;
    }

    blank3d_input_shutdown(&input);
    puts("Blank3D universal input stack test: OK");
    return 0;
}
