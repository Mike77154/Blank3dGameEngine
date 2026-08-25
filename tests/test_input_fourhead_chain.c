#include "input_keys89.h"
#include "input_hook.h"
#include "input_hook_backend_polls89.h"
#include "input_scanner.h"

#include <stdio.h>
#include <string.h>

typedef struct FakePolls89Tag {
    unsigned char kb[IHK_KB_BITS_BYTES];
    int updates;
} FakePolls89;

static void fake_update(void *backend)
{
    FakePolls89 *fake;
    fake = (FakePolls89 *)backend;
    if (fake) ++fake->updates;
}

static int fake_key_down(const void *backend, input_key89 key)
{
    const FakePolls89 *fake;
    unsigned int usage;
    unsigned int byte_index;
    unsigned int mask;
    fake = (const FakePolls89 *)backend;
    if (!fake) return 0;
    if (!input_keys89_keyboard_usage(key, &usage)) return 0;
    byte_index = usage >> 3;
    mask = 1U << (usage & 7U);
    return (fake->kb[byte_index] & (unsigned char)mask) != 0U;
}

static void fake_set(FakePolls89 *fake, input_key89 key, int down)
{
    unsigned int usage;
    unsigned int byte_index;
    unsigned char mask;
    if (!fake || !input_keys89_keyboard_usage(key, &usage)) return;
    byte_index = usage >> 3;
    mask = (unsigned char)(1U << (usage & 7U));
    if (down) fake->kb[byte_index] |= mask;
    else fake->kb[byte_index] &= (unsigned char)(~mask);
}

static input_bits_t hook_bank_bits(const input_hook *hook, int bank)
{
    input_bits_t bits;
    int bit;
    int usage;
    bits = (input_bits_t)0;
    for (bit = 0; bit < 32; ++bit) {
        usage = bank * 32 + bit;
        if (input_hook_kb_down(hook, (ihk_u8)usage))
            bits |= ((input_bits_t)1UL << bit);
    }
    return bits;
}

int main(void)
{
    FakePolls89 fake;
    ihk_polls89_key_api api;
    ihk_polls89_backend polls_to_hook;
    input_hook hook;
    InputScanner scanner;
    input_key89 up;
    input_key89 w;
    int up_bit;

    memset(&fake, 0, sizeof(fake));
    memset(&api, 0, sizeof(api));
    api.update = fake_update;
    api.key_down = fake_key_down;
    api.capabilities = IHK_CAP_LAYOUT_INDEPENDENT;

    if (ihk_polls89_backend_init(&polls_to_hook, &fake, &api) != 0)
        return 1;
    input_hook_init(&hook, ihk_polls89_backend_as_ihk(&polls_to_hook));
    if (input_scanner_init(&scanner, 0, 0) != INPUT_OK) return 2;
    if (input_scanner_set_button_count(&scanner, 32) != INPUT_OK) return 3;

    up = input_keys89_from_name("Up");
    w = input_keys89_from_name("W");
    fake_set(&fake, up, 1);
    fake_set(&fake, w, 1);

    input_hook_update(&hook);
    if (!input_hook_down(&hook, input_hook_key_from_input_key89(up))) return 4;
    if (!input_hook_pressed(&hook, input_hook_key_from_input_key89(w))) return 5;
    if (fake.updates != 1) return 6;

    /* Up is usage 0x52 -> bank 2, bit 18. */
    if (input_scanner_update_with_bits(&scanner, hook_bank_bits(&hook, 2)) != INPUT_OK)
        return 7;
    up_bit = (int)(INPUT_KEY89_USAGE(up) - 64U);
    if (!input_button_pressed(&scanner, up_bit)) return 8;

    input_hook_update(&hook);
    if (input_hook_pressed(&hook, input_hook_key_from_input_key89(up))) return 9;
    if (input_scanner_update_with_bits(&scanner, hook_bank_bits(&hook, 2)) != INPUT_OK)
        return 10;
    if (!input_button_hold(&scanner, up_bit)) return 11;

    fake_set(&fake, up, 0);
    input_hook_update(&hook);
    if (input_scanner_update_with_bits(&scanner, hook_bank_bits(&hook, 2)) != INPUT_OK)
        return 12;
    if (!input_button_released(&scanner, up_bit)) return 13;

    input_hook_shutdown(&hook);
    ihk_polls89_backend_shutdown(&polls_to_hook);
    puts("four-head input chain: input_keys89 -> polls89 -> input_hook89 -> input_scanner89: OK");
    return 0;
}
