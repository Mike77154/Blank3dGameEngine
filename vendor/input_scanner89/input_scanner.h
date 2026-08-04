#ifndef INPUT_SCANNER_H
#define INPUT_SCANNER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <limits.h>

#ifndef INPUT_BITS_TYPE
#define INPUT_BITS_TYPE unsigned long
#endif

typedef INPUT_BITS_TYPE input_bits_t;

#define INPUT_BITS_CAPACITY ((int)(sizeof(input_bits_t) * CHAR_BIT))

#ifndef INPUT_MAX_BUTTONS
#define INPUT_MAX_BUTTONS INPUT_BITS_CAPACITY
#endif

enum input_result {
    INPUT_OK = 0,
    INPUT_ERR_NULL = -1,
    INPUT_ERR_NO_BACKEND = -2,
    INPUT_ERR_BAD_INDEX = -3
};

typedef int (*InputBackendPollFn)(void *user_data, input_bits_t *out_bits);

typedef struct InputScanner {
    input_bits_t current_bits;
    input_bits_t previous_bits;
    input_bits_t pressed_bits;
    input_bits_t released_bits;
    input_bits_t valid_mask;
    unsigned short down_frames[INPUT_MAX_BUTTONS];
    unsigned short up_frames[INPUT_MAX_BUTTONS];
    unsigned short last_down_frames[INPUT_MAX_BUTTONS];
    InputBackendPollFn poll_fn;
    void *backend_user_data;
} InputScanner;

input_bits_t input_button_bit(int button_index);
input_bits_t input_mask_from_button_count(int button_count);

int input_scanner_init(InputScanner *s,
                       InputBackendPollFn fn,
                       void *user_data);
int input_scanner_reset(InputScanner *s);
int input_scanner_set_backend(InputScanner *s,
                              InputBackendPollFn fn,
                              void *user_data);
int input_scanner_set_valid_mask(InputScanner *s, input_bits_t valid_mask);
int input_scanner_set_button_count(InputScanner *s, int button_count);
int input_scanner_update(InputScanner *s);
int input_scanner_update_with_bits(InputScanner *s, input_bits_t new_bits);

int input_button_is_valid(const InputScanner *s, int button_index);
int input_button_down(const InputScanner *s, int button_index);
int input_button_hold(const InputScanner *s, int button_index);
int input_button_pressed(const InputScanner *s, int button_index);
int input_button_released(const InputScanner *s, int button_index);
int input_button_repeat(const InputScanner *s,
                        int button_index,
                        unsigned short delay_frames,
                        unsigned short interval_frames);
int input_button_long_press(const InputScanner *s,
                            int button_index,
                            unsigned short frames);
int input_button_long_hold(const InputScanner *s,
                           int button_index,
                           unsigned short frames);
int input_button_tapped(const InputScanner *s,
                        int button_index,
                        unsigned short max_down_frames);

unsigned short input_button_down_frames(const InputScanner *s, int button_index);
unsigned short input_button_up_frames(const InputScanner *s, int button_index);
unsigned short input_button_last_down_frames(const InputScanner *s, int button_index);

#ifdef __cplusplus
}
#endif

#endif /* INPUT_SCANNER_H */
