#ifndef INPUT_KEYS89_H
#define INPUT_KEYS89_H

#ifdef __cplusplus
extern "C" {
#endif

/* input_keys89 - stateless C89 key vocabulary.
 *
 * Scope:
 *   - symbolic keyboard key names and aliases
 *   - USB HID page/usage identity
 *   - deterministic name <-> key conversion
 *
 * Deliberately NOT in scope:
 *   - polling/backends
 *   - pressed/held/released state
 *   - gameplay actions/bindings
 *   - dynamic allocation
 */

typedef unsigned short input_key89;

#define INPUT_KEYS89_VERSION_MAJOR 1
#define INPUT_KEYS89_VERSION_MINOR 0
#define INPUT_KEYS89_VERSION_PATCH 0

#define INPUT_KEY89_NONE ((input_key89)0U)
#define INPUT_KEY89_PAGE_KEYBOARD 0x07U
#define INPUT_KEY89_MAKE(page, usage) \
    ((input_key89)((((unsigned short)(page)) << 8) | \
                   ((unsigned short)(usage) & 0x00FFU)))
#define INPUT_KEY89_KB(usage) INPUT_KEY89_MAKE(INPUT_KEY89_PAGE_KEYBOARD, (usage))
#define INPUT_KEY89_PAGE(key) ((unsigned int)(((key) >> 8) & 0x00FFU))
#define INPUT_KEY89_USAGE(key) ((unsigned int)((key) & 0x00FFU))

#define INPUT_KEY89_ENTER       INPUT_KEY89_KB(0x28U)
#define INPUT_KEY89_ESCAPE      INPUT_KEY89_KB(0x29U)
#define INPUT_KEY89_BACKSPACE   INPUT_KEY89_KB(0x2AU)
#define INPUT_KEY89_TAB         INPUT_KEY89_KB(0x2BU)
#define INPUT_KEY89_SPACE       INPUT_KEY89_KB(0x2CU)
#define INPUT_KEY89_PRINTSCREEN INPUT_KEY89_KB(0x46U)
#define INPUT_KEY89_INSERT      INPUT_KEY89_KB(0x49U)
#define INPUT_KEY89_HOME        INPUT_KEY89_KB(0x4AU)
#define INPUT_KEY89_PAGE_UP     INPUT_KEY89_KB(0x4BU)
#define INPUT_KEY89_DELETE      INPUT_KEY89_KB(0x4CU)
#define INPUT_KEY89_END         INPUT_KEY89_KB(0x4DU)
#define INPUT_KEY89_PAGE_DOWN   INPUT_KEY89_KB(0x4EU)
#define INPUT_KEY89_RIGHT       INPUT_KEY89_KB(0x4FU)
#define INPUT_KEY89_LEFT        INPUT_KEY89_KB(0x50U)
#define INPUT_KEY89_DOWN        INPUT_KEY89_KB(0x51U)
#define INPUT_KEY89_UP          INPUT_KEY89_KB(0x52U)
#define INPUT_KEY89_LCTRL       INPUT_KEY89_KB(0xE0U)
#define INPUT_KEY89_LSHIFT      INPUT_KEY89_KB(0xE1U)
#define INPUT_KEY89_LALT        INPUT_KEY89_KB(0xE2U)
#define INPUT_KEY89_LGUI        INPUT_KEY89_KB(0xE3U)
#define INPUT_KEY89_RCTRL       INPUT_KEY89_KB(0xE4U)
#define INPUT_KEY89_RSHIFT      INPUT_KEY89_KB(0xE5U)
#define INPUT_KEY89_RALT        INPUT_KEY89_KB(0xE6U)
#define INPUT_KEY89_RGUI        INPUT_KEY89_KB(0xE7U)

input_key89 input_keys89_from_name(const char *name);
input_key89 input_keys89_from_hid(unsigned int page, unsigned int usage);
int input_keys89_to_hid(input_key89 key,
                        unsigned int *out_page,
                        unsigned int *out_usage);
int input_keys89_is_keyboard(input_key89 key);
int input_keys89_keyboard_usage(input_key89 key, unsigned int *out_usage);

/* Returns a stable literal for named special keys, or writes generated names
 * (letters, digits, F keys, keypad digits, generic HID fallback) into tmp.
 * tmp may be NULL when only literal names are expected. */
const char *input_keys89_name(input_key89 key,
                              char *tmp,
                              unsigned int tmp_size);

#ifdef __cplusplus
}
#endif

#endif
