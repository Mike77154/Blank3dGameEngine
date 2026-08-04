#include "input_backend_pad.h"

#define INPUT_PAD_BIT(button_index) input_button_bit((int)(button_index))

static input_bits_t input_pad_mask_basic_directions(void)
{
    return INPUT_PAD_BIT(INPUT_PAD_UP) |
           INPUT_PAD_BIT(INPUT_PAD_DOWN) |
           INPUT_PAD_BIT(INPUT_PAD_LEFT) |
           INPUT_PAD_BIT(INPUT_PAD_RIGHT);
}

static input_bits_t input_pad_mask_face4(void)
{
    return INPUT_PAD_BIT(INPUT_PAD_FACE_SOUTH) |
           INPUT_PAD_BIT(INPUT_PAD_FACE_EAST) |
           INPUT_PAD_BIT(INPUT_PAD_FACE_WEST) |
           INPUT_PAD_BIT(INPUT_PAD_FACE_NORTH);
}

static input_bits_t input_pad_mask_select_start_home(void)
{
    return INPUT_PAD_BIT(INPUT_PAD_SELECT) |
           INPUT_PAD_BIT(INPUT_PAD_START) |
           INPUT_PAD_BIT(INPUT_PAD_HOME);
}

static input_bits_t input_pad_mask_shoulders(void)
{
    return INPUT_PAD_BIT(INPUT_PAD_L1) |
           INPUT_PAD_BIT(INPUT_PAD_R1) |
           INPUT_PAD_BIT(INPUT_PAD_L2) |
           INPUT_PAD_BIT(INPUT_PAD_R2) |
           INPUT_PAD_BIT(INPUT_PAD_L3) |
           INPUT_PAD_BIT(INPUT_PAD_R3);
}

static input_bits_t input_pad_mask_ccluster(void)
{
    return INPUT_PAD_BIT(INPUT_PAD_C_UP) |
           INPUT_PAD_BIT(INPUT_PAD_C_DOWN) |
           INPUT_PAD_BIT(INPUT_PAD_C_LEFT) |
           INPUT_PAD_BIT(INPUT_PAD_C_RIGHT);
}

static input_bits_t input_pad_mask_paddles(void)
{
    return INPUT_PAD_BIT(INPUT_PAD_PADDLE1) |
           INPUT_PAD_BIT(INPUT_PAD_PADDLE2) |
           INPUT_PAD_BIT(INPUT_PAD_PADDLE3) |
           INPUT_PAD_BIT(INPUT_PAD_PADDLE4);
}

static int input_pad_backend_apply(InputPadBackend *backend,
                                   input_bits_t valid_mask,
                                   int map_hat_to_dpad,
                                   int map_left_stick_to_dpad,
                                   int map_right_stick_to_cpad,
                                   int map_triggers_to_buttons)
{
    if (!backend) {
        return INPUT_ERR_NULL;
    }

    backend->snapshot.buttons_down = (input_bits_t)0;
    backend->snapshot.left_x = 0;
    backend->snapshot.left_y = 0;
    backend->snapshot.right_x = 0;
    backend->snapshot.right_y = 0;
    backend->snapshot.left_trigger = 0U;
    backend->snapshot.right_trigger = 0U;
    backend->snapshot.hat_x = 0;
    backend->snapshot.hat_y = 0;

    backend->valid_mask = valid_mask;
    backend->stick_threshold = (short)16000;
    backend->trigger_threshold = (unsigned short)16384U;
    backend->map_hat_to_dpad = (unsigned char)(map_hat_to_dpad ? 1U : 0U);
    backend->map_left_stick_to_dpad = (unsigned char)(map_left_stick_to_dpad ? 1U : 0U);
    backend->map_right_stick_to_cpad = (unsigned char)(map_right_stick_to_cpad ? 1U : 0U);
    backend->map_triggers_to_buttons = (unsigned char)(map_triggers_to_buttons ? 1U : 0U);
    return INPUT_OK;
}

static int input_pad_negative(short value, short threshold)
{
    return (value <= (short)(-threshold));
}

static int input_pad_positive(short value, short threshold)
{
    return (value >= threshold);
}

int input_pad_backend_init_generic(InputPadBackend *backend)
{
    return input_pad_backend_apply(backend,
                                   input_mask_from_button_count(INPUT_PAD_BUTTON_COUNT),
                                   1,
                                   0,
                                   0,
                                   1);
}

int input_pad_backend_init_atari_2600(InputPadBackend *backend)
{
    return input_pad_backend_apply(backend,
                                   input_pad_mask_basic_directions() |
                                   INPUT_PAD_BIT(INPUT_PAD_FACE_SOUTH),
                                   0,
                                   0,
                                   0,
                                   0);
}

int input_pad_backend_init_atari_7800(InputPadBackend *backend)
{
    return input_pad_backend_apply(backend,
                                   input_pad_mask_basic_directions() |
                                   INPUT_PAD_BIT(INPUT_PAD_FACE_SOUTH) |
                                   INPUT_PAD_BIT(INPUT_PAD_FACE_EAST),
                                   0,
                                   0,
                                   0,
                                   0);
}

int input_pad_backend_init_master_system(InputPadBackend *backend)
{
    return input_pad_backend_apply(backend,
                                   input_pad_mask_basic_directions() |
                                   INPUT_PAD_BIT(INPUT_PAD_START) |
                                   INPUT_PAD_BIT(INPUT_PAD_FACE_WEST) |
                                   INPUT_PAD_BIT(INPUT_PAD_FACE_EAST),
                                   0,
                                   0,
                                   0,
                                   0);
}

int input_pad_backend_init_pc_engine_2_button(InputPadBackend *backend)
{
    return input_pad_backend_apply(backend,
                                   input_pad_mask_basic_directions() |
                                   INPUT_PAD_BIT(INPUT_PAD_START) |
                                   INPUT_PAD_BIT(INPUT_PAD_SELECT) |
                                   INPUT_PAD_BIT(INPUT_PAD_FACE_WEST) |
                                   INPUT_PAD_BIT(INPUT_PAD_FACE_SOUTH),
                                   0,
                                   0,
                                   0,
                                   0);
}

int input_pad_backend_init_pc_engine_6_button(InputPadBackend *backend)
{
    return input_pad_backend_apply(backend,
                                   input_pad_mask_basic_directions() |
                                   INPUT_PAD_BIT(INPUT_PAD_START) |
                                   INPUT_PAD_BIT(INPUT_PAD_SELECT) |
                                   input_pad_mask_face4() |
                                   INPUT_PAD_BIT(INPUT_PAD_L1) |
                                   INPUT_PAD_BIT(INPUT_PAD_R1),
                                   0,
                                   0,
                                   0,
                                   0);
}

int input_pad_backend_init_nes(InputPadBackend *backend)
{
    return input_pad_backend_apply(backend,
                                   input_pad_mask_basic_directions() |
                                   INPUT_PAD_BIT(INPUT_PAD_START) |
                                   INPUT_PAD_BIT(INPUT_PAD_SELECT) |
                                   INPUT_PAD_BIT(INPUT_PAD_FACE_WEST) |
                                   INPUT_PAD_BIT(INPUT_PAD_FACE_EAST),
                                   0,
                                   0,
                                   0,
                                   0);
}

int input_pad_backend_init_gameboy(InputPadBackend *backend)
{
    return input_pad_backend_init_nes(backend);
}

int input_pad_backend_init_snes(InputPadBackend *backend)
{
    return input_pad_backend_apply(backend,
                                   input_pad_mask_basic_directions() |
                                   INPUT_PAD_BIT(INPUT_PAD_START) |
                                   INPUT_PAD_BIT(INPUT_PAD_SELECT) |
                                   input_pad_mask_face4() |
                                   INPUT_PAD_BIT(INPUT_PAD_L1) |
                                   INPUT_PAD_BIT(INPUT_PAD_R1),
                                   0,
                                   0,
                                   0,
                                   0);
}

int input_pad_backend_init_genesis_3_button(InputPadBackend *backend)
{
    return input_pad_backend_apply(backend,
                                   input_pad_mask_basic_directions() |
                                   INPUT_PAD_BIT(INPUT_PAD_START) |
                                   INPUT_PAD_BIT(INPUT_PAD_FACE_WEST) |
                                   INPUT_PAD_BIT(INPUT_PAD_FACE_SOUTH) |
                                   INPUT_PAD_BIT(INPUT_PAD_FACE_EAST),
                                   0,
                                   0,
                                   0,
                                   0);
}

int input_pad_backend_init_genesis_6_button(InputPadBackend *backend)
{
    return input_pad_backend_apply(backend,
                                   input_pad_mask_basic_directions() |
                                   INPUT_PAD_BIT(INPUT_PAD_START) |
                                   INPUT_PAD_BIT(INPUT_PAD_MODE) |
                                   INPUT_PAD_BIT(INPUT_PAD_FACE_WEST) |
                                   INPUT_PAD_BIT(INPUT_PAD_FACE_SOUTH) |
                                   INPUT_PAD_BIT(INPUT_PAD_FACE_EAST) |
                                   INPUT_PAD_BIT(INPUT_PAD_FACE_NORTH) |
                                   INPUT_PAD_BIT(INPUT_PAD_L1) |
                                   INPUT_PAD_BIT(INPUT_PAD_R1),
                                   0,
                                   0,
                                   0,
                                   0);
}

int input_pad_backend_init_neogeo_4_button(InputPadBackend *backend)
{
    return input_pad_backend_apply(backend,
                                   input_pad_mask_basic_directions() |
                                   INPUT_PAD_BIT(INPUT_PAD_START) |
                                   input_pad_mask_face4(),
                                   0,
                                   0,
                                   0,
                                   0);
}

int input_pad_backend_init_saturn_6_button(InputPadBackend *backend)
{
    return input_pad_backend_apply(backend,
                                   input_pad_mask_basic_directions() |
                                   input_pad_mask_select_start_home() |
                                   input_pad_mask_face4() |
                                   INPUT_PAD_BIT(INPUT_PAD_L1) |
                                   INPUT_PAD_BIT(INPUT_PAD_R1),
                                   0,
                                   0,
                                   0,
                                   0);
}

int input_pad_backend_init_dreamcast(InputPadBackend *backend)
{
    return input_pad_backend_apply(backend,
                                   input_pad_mask_basic_directions() |
                                   INPUT_PAD_BIT(INPUT_PAD_START) |
                                   input_pad_mask_face4() |
                                   INPUT_PAD_BIT(INPUT_PAD_L2) |
                                   INPUT_PAD_BIT(INPUT_PAD_R2),
                                   0,
                                   0,
                                   0,
                                   1);
}

int input_pad_backend_init_n64(InputPadBackend *backend)
{
    return input_pad_backend_apply(backend,
                                   input_pad_mask_basic_directions() |
                                   INPUT_PAD_BIT(INPUT_PAD_START) |
                                   INPUT_PAD_BIT(INPUT_PAD_FACE_EAST) |
                                   INPUT_PAD_BIT(INPUT_PAD_FACE_WEST) |
                                   INPUT_PAD_BIT(INPUT_PAD_L1) |
                                   INPUT_PAD_BIT(INPUT_PAD_R1) |
                                   INPUT_PAD_BIT(INPUT_PAD_Z) |
                                   input_pad_mask_ccluster(),
                                   0,
                                   0,
                                   0,
                                   0);
}

int input_pad_backend_init_gamecube(InputPadBackend *backend)
{
    return input_pad_backend_apply(backend,
                                   input_pad_mask_basic_directions() |
                                   INPUT_PAD_BIT(INPUT_PAD_START) |
                                   input_pad_mask_face4() |
                                   INPUT_PAD_BIT(INPUT_PAD_L1) |
                                   INPUT_PAD_BIT(INPUT_PAD_R1) |
                                   INPUT_PAD_BIT(INPUT_PAD_L2) |
                                   INPUT_PAD_BIT(INPUT_PAD_R2) |
                                   INPUT_PAD_BIT(INPUT_PAD_Z),
                                   0,
                                   0,
                                   0,
                                   1);
}

int input_pad_backend_init_switch_pro(InputPadBackend *backend)
{
    return input_pad_backend_apply(backend,
                                   input_pad_mask_basic_directions() |
                                   input_pad_mask_select_start_home() |
                                   input_pad_mask_face4() |
                                   input_pad_mask_shoulders() |
                                   INPUT_PAD_BIT(INPUT_PAD_CAPTURE),
                                   1,
                                   0,
                                   0,
                                   1);
}

int input_pad_backend_init_joycon_pair(InputPadBackend *backend)
{
    return input_pad_backend_init_switch_pro(backend);
}

int input_pad_backend_init_wii_classic(InputPadBackend *backend)
{
    return input_pad_backend_apply(backend,
                                   input_pad_mask_basic_directions() |
                                   input_pad_mask_select_start_home() |
                                   input_pad_mask_face4() |
                                   input_pad_mask_shoulders(),
                                   1,
                                   0,
                                   0,
                                   1);
}

int input_pad_backend_init_wii_u_pro(InputPadBackend *backend)
{
    return input_pad_backend_apply(backend,
                                   input_pad_mask_basic_directions() |
                                   input_pad_mask_select_start_home() |
                                   input_pad_mask_face4() |
                                   input_pad_mask_shoulders(),
                                   1,
                                   0,
                                   0,
                                   1);
}

int input_pad_backend_init_playstation_digital(InputPadBackend *backend)
{
    return input_pad_backend_apply(backend,
                                   input_pad_mask_basic_directions() |
                                   INPUT_PAD_BIT(INPUT_PAD_START) |
                                   INPUT_PAD_BIT(INPUT_PAD_SELECT) |
                                   input_pad_mask_face4() |
                                   INPUT_PAD_BIT(INPUT_PAD_L1) |
                                   INPUT_PAD_BIT(INPUT_PAD_R1) |
                                   INPUT_PAD_BIT(INPUT_PAD_L2) |
                                   INPUT_PAD_BIT(INPUT_PAD_R2),
                                   0,
                                   0,
                                   0,
                                   0);
}

int input_pad_backend_init_dualshock(InputPadBackend *backend)
{
    return input_pad_backend_apply(backend,
                                   input_pad_mask_basic_directions() |
                                   input_pad_mask_select_start_home() |
                                   input_pad_mask_face4() |
                                   input_pad_mask_shoulders() |
                                   INPUT_PAD_BIT(INPUT_PAD_TOUCHPAD),
                                   1,
                                   0,
                                   0,
                                   1);
}

int input_pad_backend_init_psp(InputPadBackend *backend)
{
    return input_pad_backend_apply(backend,
                                   input_pad_mask_basic_directions() |
                                   INPUT_PAD_BIT(INPUT_PAD_START) |
                                   INPUT_PAD_BIT(INPUT_PAD_SELECT) |
                                   INPUT_PAD_BIT(INPUT_PAD_HOME) |
                                   input_pad_mask_face4() |
                                   INPUT_PAD_BIT(INPUT_PAD_L1) |
                                   INPUT_PAD_BIT(INPUT_PAD_R1),
                                   0,
                                   0,
                                   0,
                                   0);
}

int input_pad_backend_init_psvita(InputPadBackend *backend)
{
    return input_pad_backend_apply(backend,
                                   input_pad_mask_basic_directions() |
                                   INPUT_PAD_BIT(INPUT_PAD_START) |
                                   INPUT_PAD_BIT(INPUT_PAD_SELECT) |
                                   INPUT_PAD_BIT(INPUT_PAD_HOME) |
                                   input_pad_mask_face4() |
                                   INPUT_PAD_BIT(INPUT_PAD_L1) |
                                   INPUT_PAD_BIT(INPUT_PAD_R1) |
                                   INPUT_PAD_BIT(INPUT_PAD_TOUCHPAD),
                                   0,
                                   0,
                                   0,
                                   0);
}

int input_pad_backend_init_xinput(InputPadBackend *backend)
{
    return input_pad_backend_apply(backend,
                                   input_pad_mask_basic_directions() |
                                   input_pad_mask_select_start_home() |
                                   input_pad_mask_face4() |
                                   input_pad_mask_shoulders(),
                                   1,
                                   0,
                                   0,
                                   1);
}

int input_pad_backend_init_xbox_elite(InputPadBackend *backend)
{
    return input_pad_backend_apply(backend,
                                   input_pad_mask_basic_directions() |
                                   input_pad_mask_select_start_home() |
                                   input_pad_mask_face4() |
                                   input_pad_mask_shoulders() |
                                   input_pad_mask_paddles() |
                                   INPUT_PAD_BIT(INPUT_PAD_CAPTURE),
                                   1,
                                   0,
                                   0,
                                   1);
}

int input_pad_backend_init_arcade_2_button(InputPadBackend *backend)
{
    return input_pad_backend_apply(backend,
                                   input_pad_mask_basic_directions() |
                                   INPUT_PAD_BIT(INPUT_PAD_START) |
                                   INPUT_PAD_BIT(INPUT_PAD_SELECT) |
                                   INPUT_PAD_BIT(INPUT_PAD_FACE_WEST) |
                                   INPUT_PAD_BIT(INPUT_PAD_FACE_SOUTH),
                                   0,
                                   0,
                                   0,
                                   0);
}

int input_pad_backend_init_arcade_4_button(InputPadBackend *backend)
{
    return input_pad_backend_apply(backend,
                                   input_pad_mask_basic_directions() |
                                   INPUT_PAD_BIT(INPUT_PAD_START) |
                                   INPUT_PAD_BIT(INPUT_PAD_SELECT) |
                                   input_pad_mask_face4(),
                                   0,
                                   0,
                                   0,
                                   0);
}

int input_pad_backend_init_arcade_6_button(InputPadBackend *backend)
{
    return input_pad_backend_apply(backend,
                                   input_pad_mask_basic_directions() |
                                   INPUT_PAD_BIT(INPUT_PAD_START) |
                                   INPUT_PAD_BIT(INPUT_PAD_SELECT) |
                                   input_pad_mask_face4() |
                                   INPUT_PAD_BIT(INPUT_PAD_L1) |
                                   INPUT_PAD_BIT(INPUT_PAD_R1),
                                   0,
                                   0,
                                   0,
                                   0);
}

int input_pad_backend_init_arcade_8_button(InputPadBackend *backend)
{
    return input_pad_backend_apply(backend,
                                   input_pad_mask_basic_directions() |
                                   INPUT_PAD_BIT(INPUT_PAD_START) |
                                   INPUT_PAD_BIT(INPUT_PAD_SELECT) |
                                   input_pad_mask_face4() |
                                   INPUT_PAD_BIT(INPUT_PAD_L1) |
                                   INPUT_PAD_BIT(INPUT_PAD_R1) |
                                   INPUT_PAD_BIT(INPUT_PAD_L2) |
                                   INPUT_PAD_BIT(INPUT_PAD_R2),
                                   0,
                                   0,
                                   0,
                                   0);
}

int input_pad_backend_set_snapshot(InputPadBackend *backend,
                                   const InputPadSnapshot *snapshot)
{
    if (!backend || !snapshot) {
        return INPUT_ERR_NULL;
    }

    backend->snapshot = *snapshot;
    backend->snapshot.buttons_down &= backend->valid_mask;
    return INPUT_OK;
}

int input_pad_backend_enable_hat_to_dpad(InputPadBackend *backend, int enabled)
{
    if (!backend) {
        return INPUT_ERR_NULL;
    }

    backend->map_hat_to_dpad = (unsigned char)(enabled ? 1U : 0U);
    return INPUT_OK;
}

int input_pad_backend_enable_left_stick_to_dpad(InputPadBackend *backend, int enabled)
{
    if (!backend) {
        return INPUT_ERR_NULL;
    }

    backend->map_left_stick_to_dpad = (unsigned char)(enabled ? 1U : 0U);
    return INPUT_OK;
}

int input_pad_backend_enable_right_stick_to_cpad(InputPadBackend *backend, int enabled)
{
    if (!backend) {
        return INPUT_ERR_NULL;
    }

    backend->map_right_stick_to_cpad = (unsigned char)(enabled ? 1U : 0U);
    return INPUT_OK;
}

int input_pad_backend_enable_triggers_to_buttons(InputPadBackend *backend, int enabled)
{
    if (!backend) {
        return INPUT_ERR_NULL;
    }

    backend->map_triggers_to_buttons = (unsigned char)(enabled ? 1U : 0U);
    return INPUT_OK;
}

int input_pad_backend_set_thresholds(InputPadBackend *backend,
                                     short stick_threshold,
                                     unsigned short trigger_threshold)
{
    if (!backend) {
        return INPUT_ERR_NULL;
    }

    if (stick_threshold < 0) {
        stick_threshold = (short)(-stick_threshold);
    }

    backend->stick_threshold = stick_threshold;
    backend->trigger_threshold = trigger_threshold;
    return INPUT_OK;
}

short input_pad_backend_left_x(const InputPadBackend *backend)
{
    if (!backend) {
        return 0;
    }

    return backend->snapshot.left_x;
}

short input_pad_backend_left_y(const InputPadBackend *backend)
{
    if (!backend) {
        return 0;
    }

    return backend->snapshot.left_y;
}

short input_pad_backend_right_x(const InputPadBackend *backend)
{
    if (!backend) {
        return 0;
    }

    return backend->snapshot.right_x;
}

short input_pad_backend_right_y(const InputPadBackend *backend)
{
    if (!backend) {
        return 0;
    }

    return backend->snapshot.right_y;
}

unsigned short input_pad_backend_left_trigger(const InputPadBackend *backend)
{
    if (!backend) {
        return 0U;
    }

    return backend->snapshot.left_trigger;
}

unsigned short input_pad_backend_right_trigger(const InputPadBackend *backend)
{
    if (!backend) {
        return 0U;
    }

    return backend->snapshot.right_trigger;
}

int input_pad_backend_poll(void *user_data, input_bits_t *out_bits)
{
    InputPadBackend *backend;
    input_bits_t bits;

    if (!user_data || !out_bits) {
        return INPUT_ERR_NULL;
    }

    backend = (InputPadBackend *)user_data;
    bits = backend->snapshot.buttons_down & backend->valid_mask;

    if (backend->map_hat_to_dpad != 0U) {
        if (backend->snapshot.hat_x < 0) {
            bits |= INPUT_PAD_BIT(INPUT_PAD_LEFT);
        } else if (backend->snapshot.hat_x > 0) {
            bits |= INPUT_PAD_BIT(INPUT_PAD_RIGHT);
        }

        if (backend->snapshot.hat_y < 0) {
            bits |= INPUT_PAD_BIT(INPUT_PAD_UP);
        } else if (backend->snapshot.hat_y > 0) {
            bits |= INPUT_PAD_BIT(INPUT_PAD_DOWN);
        }
    }

    if (backend->map_left_stick_to_dpad != 0U) {
        if (input_pad_negative(backend->snapshot.left_x, backend->stick_threshold)) {
            bits |= INPUT_PAD_BIT(INPUT_PAD_LEFT);
        } else if (input_pad_positive(backend->snapshot.left_x, backend->stick_threshold)) {
            bits |= INPUT_PAD_BIT(INPUT_PAD_RIGHT);
        }

        if (input_pad_negative(backend->snapshot.left_y, backend->stick_threshold)) {
            bits |= INPUT_PAD_BIT(INPUT_PAD_UP);
        } else if (input_pad_positive(backend->snapshot.left_y, backend->stick_threshold)) {
            bits |= INPUT_PAD_BIT(INPUT_PAD_DOWN);
        }
    }

    if (backend->map_right_stick_to_cpad != 0U) {
        if (input_pad_negative(backend->snapshot.right_x, backend->stick_threshold)) {
            bits |= INPUT_PAD_BIT(INPUT_PAD_C_LEFT);
        } else if (input_pad_positive(backend->snapshot.right_x, backend->stick_threshold)) {
            bits |= INPUT_PAD_BIT(INPUT_PAD_C_RIGHT);
        }

        if (input_pad_negative(backend->snapshot.right_y, backend->stick_threshold)) {
            bits |= INPUT_PAD_BIT(INPUT_PAD_C_UP);
        } else if (input_pad_positive(backend->snapshot.right_y, backend->stick_threshold)) {
            bits |= INPUT_PAD_BIT(INPUT_PAD_C_DOWN);
        }
    }

    if (backend->map_triggers_to_buttons != 0U) {
        if (backend->snapshot.left_trigger >= backend->trigger_threshold) {
            bits |= INPUT_PAD_BIT(INPUT_PAD_L2);
        }
        if (backend->snapshot.right_trigger >= backend->trigger_threshold) {
            bits |= INPUT_PAD_BIT(INPUT_PAD_R2);
        }
    }

    *out_bits = bits & backend->valid_mask;
    return INPUT_OK;
}
