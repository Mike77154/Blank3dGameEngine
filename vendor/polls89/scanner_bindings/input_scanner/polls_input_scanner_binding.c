#include <string.h>

#include "polls_input_scanner_binding.h"

/* input_scanner espera un poll_fn que llene un bitmask (input_bits_t).
 * polls, en cambio, expone un callback por botón.
 *
 * Este adapter construye el bitmask consultando state_fn(i) para cada botón.
 */

static void input_scanner_poll_adapter(void *user_data, input_bits_t *out_bits)
{
    polls_input_scanner_binding *binding = (polls_input_scanner_binding*)user_data;
    input_bits_t bits = 0;
    int i;

    if (!out_bits) return;
    *out_bits = 0;

    if (!binding || !binding->state_fn) {
        return;
    }

    for (i = 0; i < (int)INPUT_MAX_BUTTONS; ++i) {
        if (binding->state_fn(binding->state_user_data, i)) {
            bits |= ((input_bits_t)1u) << i;
        }
    }

    *out_bits = bits;
}

static int adapter_connect(void *scanner, polls_button_state_fn state_fn, void *state_user_data)
{
    polls_input_scanner_binding *binding = (polls_input_scanner_binding*)scanner;
    if (!binding) return -1;

    binding->state_fn = state_fn;
    binding->state_user_data = state_user_data;

    input_scanner_init(&binding->scanner, input_scanner_poll_adapter, binding);
    return 0;
}

static void adapter_update(void *scanner)
{
    polls_input_scanner_binding *binding = (polls_input_scanner_binding*)scanner;
    if (!binding) return;
    input_scanner_update(&binding->scanner);
}

static int adapter_hold(const void *scanner, int button_index)
{
    const polls_input_scanner_binding *binding = (const polls_input_scanner_binding*)scanner;
    if (!binding) return 0;
    return input_button_hold(&binding->scanner, button_index);
}

static int adapter_pressed(const void *scanner, int button_index)
{
    const polls_input_scanner_binding *binding = (const polls_input_scanner_binding*)scanner;
    if (!binding) return 0;
    return input_button_pressed(&binding->scanner, button_index);
}

static int adapter_released(const void *scanner, int button_index)
{
    const polls_input_scanner_binding *binding = (const polls_input_scanner_binding*)scanner;
    if (!binding) return 0;
    return input_button_released(&binding->scanner, button_index);
}

polls_scanner_iface polls_make_input_scanner_iface(polls_input_scanner_binding *binding)
{
    polls_scanner_iface iface;

    /* inicializar wrapper a cero por seguridad */
    if (binding) {
        memset(binding, 0, sizeof(*binding));
    }

    iface.scanner = binding;
    iface.max_buttons = INPUT_MAX_BUTTONS;
    iface.connect = adapter_connect;
    iface.update = adapter_update;
    iface.hold = adapter_hold;
    iface.pressed = adapter_pressed;
    iface.released = adapter_released;
    return iface;
}
