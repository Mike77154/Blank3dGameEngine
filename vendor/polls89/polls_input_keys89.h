#ifndef POLLS_INPUT_KEYS89_H
#define POLLS_INPUT_KEYS89_H

#ifdef __cplusplus
extern "C" {
#endif

#include "key_pc.h"
#include "input_keys89.h"

/* Bridge between polls89's legacy key_pc_code ABI and the canonical
 * input_keys89 HID identity.  This keeps existing polls89 consumers working
 * while giving all four input heads one shared key vocabulary. */

input_key89 polls_key_pc_to_input_key89(key_pc_code key);
key_pc_code polls_key_pc_from_input_key89(input_key89 key);
int polls_key_pc_to_keyboard_usage(key_pc_code key, unsigned int *out_usage);

#ifdef __cplusplus
}
#endif

#endif
