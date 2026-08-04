#ifndef INPUT_BACKEND_PAD_H
#define INPUT_BACKEND_PAD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "input_scanner.h"

#define INPUT_PAD_BUTTON_COUNT 32

enum input_pad_button {
    INPUT_PAD_UP = 0,
    INPUT_PAD_DOWN = 1,
    INPUT_PAD_LEFT = 2,
    INPUT_PAD_RIGHT = 3,
    INPUT_PAD_START = 4,
    INPUT_PAD_SELECT = 5,
    INPUT_PAD_HOME = 6,
    INPUT_PAD_FACE_SOUTH = 7,
    INPUT_PAD_FACE_EAST = 8,
    INPUT_PAD_FACE_WEST = 9,
    INPUT_PAD_FACE_NORTH = 10,
    INPUT_PAD_L1 = 11,
    INPUT_PAD_R1 = 12,
    INPUT_PAD_L2 = 13,
    INPUT_PAD_R2 = 14,
    INPUT_PAD_L3 = 15,
    INPUT_PAD_R3 = 16,
    INPUT_PAD_C_UP = 17,
    INPUT_PAD_C_DOWN = 18,
    INPUT_PAD_C_LEFT = 19,
    INPUT_PAD_C_RIGHT = 20,
    INPUT_PAD_Z = 21,
    INPUT_PAD_MODE = 22,
    INPUT_PAD_CAPTURE = 23,
    INPUT_PAD_TOUCHPAD = 24,
    INPUT_PAD_PADDLE1 = 25,
    INPUT_PAD_PADDLE2 = 26,
    INPUT_PAD_PADDLE3 = 27,
    INPUT_PAD_PADDLE4 = 28,
    INPUT_PAD_MISC1 = 29,
    INPUT_PAD_MISC2 = 30,
    INPUT_PAD_MISC3 = 31
};


enum input_atari_button {
    INPUT_ATARI_UP = INPUT_PAD_UP,
    INPUT_ATARI_DOWN = INPUT_PAD_DOWN,
    INPUT_ATARI_LEFT = INPUT_PAD_LEFT,
    INPUT_ATARI_RIGHT = INPUT_PAD_RIGHT,
    INPUT_ATARI_FIRE1 = INPUT_PAD_FACE_SOUTH,
    INPUT_ATARI_FIRE2 = INPUT_PAD_FACE_EAST
};

enum input_master_system_button {
    INPUT_SMS_UP = INPUT_PAD_UP,
    INPUT_SMS_DOWN = INPUT_PAD_DOWN,
    INPUT_SMS_LEFT = INPUT_PAD_LEFT,
    INPUT_SMS_RIGHT = INPUT_PAD_RIGHT,
    INPUT_SMS_START = INPUT_PAD_START,
    INPUT_SMS_1 = INPUT_PAD_FACE_WEST,
    INPUT_SMS_2 = INPUT_PAD_FACE_EAST
};

enum input_pc_engine_button {
    INPUT_PCE_UP = INPUT_PAD_UP,
    INPUT_PCE_DOWN = INPUT_PAD_DOWN,
    INPUT_PCE_LEFT = INPUT_PAD_LEFT,
    INPUT_PCE_RIGHT = INPUT_PAD_RIGHT,
    INPUT_PCE_RUN = INPUT_PAD_START,
    INPUT_PCE_SELECT = INPUT_PAD_SELECT,
    INPUT_PCE_I = INPUT_PAD_FACE_WEST,
    INPUT_PCE_II = INPUT_PAD_FACE_SOUTH,
    INPUT_PCE_III = INPUT_PAD_FACE_EAST,
    INPUT_PCE_IV = INPUT_PAD_FACE_NORTH,
    INPUT_PCE_V = INPUT_PAD_L1,
    INPUT_PCE_VI = INPUT_PAD_R1
};

enum input_nes_button {
    INPUT_NES_UP = INPUT_PAD_UP,
    INPUT_NES_DOWN = INPUT_PAD_DOWN,
    INPUT_NES_LEFT = INPUT_PAD_LEFT,
    INPUT_NES_RIGHT = INPUT_PAD_RIGHT,
    INPUT_NES_START = INPUT_PAD_START,
    INPUT_NES_SELECT = INPUT_PAD_SELECT,
    INPUT_NES_B = INPUT_PAD_FACE_WEST,
    INPUT_NES_A = INPUT_PAD_FACE_EAST
};

enum input_gameboy_button {
    INPUT_GB_UP = INPUT_PAD_UP,
    INPUT_GB_DOWN = INPUT_PAD_DOWN,
    INPUT_GB_LEFT = INPUT_PAD_LEFT,
    INPUT_GB_RIGHT = INPUT_PAD_RIGHT,
    INPUT_GB_START = INPUT_PAD_START,
    INPUT_GB_SELECT = INPUT_PAD_SELECT,
    INPUT_GB_B = INPUT_PAD_FACE_WEST,
    INPUT_GB_A = INPUT_PAD_FACE_EAST
};

enum input_snes_button {
    INPUT_SNES_UP = INPUT_PAD_UP,
    INPUT_SNES_DOWN = INPUT_PAD_DOWN,
    INPUT_SNES_LEFT = INPUT_PAD_LEFT,
    INPUT_SNES_RIGHT = INPUT_PAD_RIGHT,
    INPUT_SNES_START = INPUT_PAD_START,
    INPUT_SNES_SELECT = INPUT_PAD_SELECT,
    INPUT_SNES_B = INPUT_PAD_FACE_SOUTH,
    INPUT_SNES_A = INPUT_PAD_FACE_EAST,
    INPUT_SNES_Y = INPUT_PAD_FACE_WEST,
    INPUT_SNES_X = INPUT_PAD_FACE_NORTH,
    INPUT_SNES_L = INPUT_PAD_L1,
    INPUT_SNES_R = INPUT_PAD_R1
};

enum input_genesis_button {
    INPUT_GENESIS_UP = INPUT_PAD_UP,
    INPUT_GENESIS_DOWN = INPUT_PAD_DOWN,
    INPUT_GENESIS_LEFT = INPUT_PAD_LEFT,
    INPUT_GENESIS_RIGHT = INPUT_PAD_RIGHT,
    INPUT_GENESIS_START = INPUT_PAD_START,
    INPUT_GENESIS_A = INPUT_PAD_FACE_WEST,
    INPUT_GENESIS_B = INPUT_PAD_FACE_SOUTH,
    INPUT_GENESIS_C = INPUT_PAD_FACE_EAST,
    INPUT_GENESIS_X = INPUT_PAD_L1,
    INPUT_GENESIS_Y = INPUT_PAD_FACE_NORTH,
    INPUT_GENESIS_Z = INPUT_PAD_R1,
    INPUT_GENESIS_MODE = INPUT_PAD_MODE
};

enum input_neogeo_button {
    INPUT_NEOGEO_UP = INPUT_PAD_UP,
    INPUT_NEOGEO_DOWN = INPUT_PAD_DOWN,
    INPUT_NEOGEO_LEFT = INPUT_PAD_LEFT,
    INPUT_NEOGEO_RIGHT = INPUT_PAD_RIGHT,
    INPUT_NEOGEO_START = INPUT_PAD_START,
    INPUT_NEOGEO_A = INPUT_PAD_FACE_WEST,
    INPUT_NEOGEO_B = INPUT_PAD_FACE_SOUTH,
    INPUT_NEOGEO_C = INPUT_PAD_FACE_NORTH,
    INPUT_NEOGEO_D = INPUT_PAD_FACE_EAST
};

enum input_n64_button {
    INPUT_N64_UP = INPUT_PAD_UP,
    INPUT_N64_DOWN = INPUT_PAD_DOWN,
    INPUT_N64_LEFT = INPUT_PAD_LEFT,
    INPUT_N64_RIGHT = INPUT_PAD_RIGHT,
    INPUT_N64_START = INPUT_PAD_START,
    INPUT_N64_A = INPUT_PAD_FACE_EAST,
    INPUT_N64_B = INPUT_PAD_FACE_WEST,
    INPUT_N64_L = INPUT_PAD_L1,
    INPUT_N64_R = INPUT_PAD_R1,
    INPUT_N64_Z = INPUT_PAD_Z,
    INPUT_N64_C_UP = INPUT_PAD_C_UP,
    INPUT_N64_C_DOWN = INPUT_PAD_C_DOWN,
    INPUT_N64_C_LEFT = INPUT_PAD_C_LEFT,
    INPUT_N64_C_RIGHT = INPUT_PAD_C_RIGHT
};

enum input_gamecube_button {
    INPUT_GC_UP = INPUT_PAD_UP,
    INPUT_GC_DOWN = INPUT_PAD_DOWN,
    INPUT_GC_LEFT = INPUT_PAD_LEFT,
    INPUT_GC_RIGHT = INPUT_PAD_RIGHT,
    INPUT_GC_START = INPUT_PAD_START,
    INPUT_GC_A = INPUT_PAD_FACE_SOUTH,
    INPUT_GC_B = INPUT_PAD_FACE_WEST,
    INPUT_GC_X = INPUT_PAD_FACE_NORTH,
    INPUT_GC_Y = INPUT_PAD_FACE_EAST,
    INPUT_GC_L = INPUT_PAD_L1,
    INPUT_GC_R = INPUT_PAD_R1,
    INPUT_GC_Z = INPUT_PAD_Z,
    INPUT_GC_LT = INPUT_PAD_L2,
    INPUT_GC_RT = INPUT_PAD_R2
};

enum input_playstation_button {
    INPUT_PS_UP = INPUT_PAD_UP,
    INPUT_PS_DOWN = INPUT_PAD_DOWN,
    INPUT_PS_LEFT = INPUT_PAD_LEFT,
    INPUT_PS_RIGHT = INPUT_PAD_RIGHT,
    INPUT_PS_START = INPUT_PAD_START,
    INPUT_PS_SELECT = INPUT_PAD_SELECT,
    INPUT_PS_HOME = INPUT_PAD_HOME,
    INPUT_PS_CROSS = INPUT_PAD_FACE_SOUTH,
    INPUT_PS_CIRCLE = INPUT_PAD_FACE_EAST,
    INPUT_PS_SQUARE = INPUT_PAD_FACE_WEST,
    INPUT_PS_TRIANGLE = INPUT_PAD_FACE_NORTH,
    INPUT_PS_L1 = INPUT_PAD_L1,
    INPUT_PS_R1 = INPUT_PAD_R1,
    INPUT_PS_L2 = INPUT_PAD_L2,
    INPUT_PS_R2 = INPUT_PAD_R2,
    INPUT_PS_L3 = INPUT_PAD_L3,
    INPUT_PS_R3 = INPUT_PAD_R3,
    INPUT_PS_TOUCHPAD = INPUT_PAD_TOUCHPAD
};

enum input_xbox_button {
    INPUT_XBOX_UP = INPUT_PAD_UP,
    INPUT_XBOX_DOWN = INPUT_PAD_DOWN,
    INPUT_XBOX_LEFT = INPUT_PAD_LEFT,
    INPUT_XBOX_RIGHT = INPUT_PAD_RIGHT,
    INPUT_XBOX_START = INPUT_PAD_START,
    INPUT_XBOX_BACK = INPUT_PAD_SELECT,
    INPUT_XBOX_GUIDE = INPUT_PAD_HOME,
    INPUT_XBOX_A = INPUT_PAD_FACE_SOUTH,
    INPUT_XBOX_B = INPUT_PAD_FACE_EAST,
    INPUT_XBOX_X = INPUT_PAD_FACE_WEST,
    INPUT_XBOX_Y = INPUT_PAD_FACE_NORTH,
    INPUT_XBOX_LB = INPUT_PAD_L1,
    INPUT_XBOX_RB = INPUT_PAD_R1,
    INPUT_XBOX_LT = INPUT_PAD_L2,
    INPUT_XBOX_RT = INPUT_PAD_R2,
    INPUT_XBOX_LS = INPUT_PAD_L3,
    INPUT_XBOX_RS = INPUT_PAD_R3,
    INPUT_XBOX_P1 = INPUT_PAD_PADDLE1,
    INPUT_XBOX_P2 = INPUT_PAD_PADDLE2,
    INPUT_XBOX_P3 = INPUT_PAD_PADDLE3,
    INPUT_XBOX_P4 = INPUT_PAD_PADDLE4,
    INPUT_XBOX_CAPTURE = INPUT_PAD_CAPTURE
};



enum input_switch_button {
    INPUT_SWITCH_UP = INPUT_PAD_UP,
    INPUT_SWITCH_DOWN = INPUT_PAD_DOWN,
    INPUT_SWITCH_LEFT = INPUT_PAD_LEFT,
    INPUT_SWITCH_RIGHT = INPUT_PAD_RIGHT,
    INPUT_SWITCH_PLUS = INPUT_PAD_START,
    INPUT_SWITCH_MINUS = INPUT_PAD_SELECT,
    INPUT_SWITCH_HOME = INPUT_PAD_HOME,
    INPUT_SWITCH_CAPTURE = INPUT_PAD_CAPTURE,
    INPUT_SWITCH_B = INPUT_PAD_FACE_SOUTH,
    INPUT_SWITCH_A = INPUT_PAD_FACE_EAST,
    INPUT_SWITCH_Y = INPUT_PAD_FACE_WEST,
    INPUT_SWITCH_X = INPUT_PAD_FACE_NORTH,
    INPUT_SWITCH_L = INPUT_PAD_L1,
    INPUT_SWITCH_R = INPUT_PAD_R1,
    INPUT_SWITCH_ZL = INPUT_PAD_L2,
    INPUT_SWITCH_ZR = INPUT_PAD_R2,
    INPUT_SWITCH_LS = INPUT_PAD_L3,
    INPUT_SWITCH_RS = INPUT_PAD_R3
};

enum input_wii_classic_button {
    INPUT_WII_CLASSIC_UP = INPUT_PAD_UP,
    INPUT_WII_CLASSIC_DOWN = INPUT_PAD_DOWN,
    INPUT_WII_CLASSIC_LEFT = INPUT_PAD_LEFT,
    INPUT_WII_CLASSIC_RIGHT = INPUT_PAD_RIGHT,
    INPUT_WII_CLASSIC_PLUS = INPUT_PAD_START,
    INPUT_WII_CLASSIC_MINUS = INPUT_PAD_SELECT,
    INPUT_WII_CLASSIC_HOME = INPUT_PAD_HOME,
    INPUT_WII_CLASSIC_B = INPUT_PAD_FACE_SOUTH,
    INPUT_WII_CLASSIC_A = INPUT_PAD_FACE_EAST,
    INPUT_WII_CLASSIC_Y = INPUT_PAD_FACE_WEST,
    INPUT_WII_CLASSIC_X = INPUT_PAD_FACE_NORTH,
    INPUT_WII_CLASSIC_L = INPUT_PAD_L1,
    INPUT_WII_CLASSIC_R = INPUT_PAD_R1,
    INPUT_WII_CLASSIC_ZL = INPUT_PAD_L2,
    INPUT_WII_CLASSIC_ZR = INPUT_PAD_R2,
    INPUT_WII_CLASSIC_LS = INPUT_PAD_L3,
    INPUT_WII_CLASSIC_RS = INPUT_PAD_R3
};

enum input_arcade_button {
    INPUT_ARCADE_UP = INPUT_PAD_UP,
    INPUT_ARCADE_DOWN = INPUT_PAD_DOWN,
    INPUT_ARCADE_LEFT = INPUT_PAD_LEFT,
    INPUT_ARCADE_RIGHT = INPUT_PAD_RIGHT,
    INPUT_ARCADE_START = INPUT_PAD_START,
    INPUT_ARCADE_COIN = INPUT_PAD_SELECT,
    INPUT_ARCADE_B1 = INPUT_PAD_FACE_WEST,
    INPUT_ARCADE_B2 = INPUT_PAD_FACE_SOUTH,
    INPUT_ARCADE_B3 = INPUT_PAD_FACE_EAST,
    INPUT_ARCADE_B4 = INPUT_PAD_FACE_NORTH,
    INPUT_ARCADE_B5 = INPUT_PAD_L1,
    INPUT_ARCADE_B6 = INPUT_PAD_R1,
    INPUT_ARCADE_B7 = INPUT_PAD_L2,
    INPUT_ARCADE_B8 = INPUT_PAD_R2
};

typedef struct InputPadSnapshot {
    input_bits_t buttons_down;
    short left_x;
    short left_y;
    short right_x;
    short right_y;
    unsigned short left_trigger;
    unsigned short right_trigger;
    signed char hat_x;
    signed char hat_y;
} InputPadSnapshot;

typedef struct InputPadBackend {
    InputPadSnapshot snapshot;
    input_bits_t valid_mask;
    short stick_threshold;
    unsigned short trigger_threshold;
    unsigned char map_hat_to_dpad;
    unsigned char map_left_stick_to_dpad;
    unsigned char map_right_stick_to_cpad;
    unsigned char map_triggers_to_buttons;
} InputPadBackend;

int input_pad_backend_init_generic(InputPadBackend *backend);
int input_pad_backend_init_atari_2600(InputPadBackend *backend);
int input_pad_backend_init_atari_7800(InputPadBackend *backend);
int input_pad_backend_init_master_system(InputPadBackend *backend);
int input_pad_backend_init_pc_engine_2_button(InputPadBackend *backend);
int input_pad_backend_init_pc_engine_6_button(InputPadBackend *backend);
int input_pad_backend_init_nes(InputPadBackend *backend);
int input_pad_backend_init_gameboy(InputPadBackend *backend);
int input_pad_backend_init_snes(InputPadBackend *backend);
int input_pad_backend_init_genesis_3_button(InputPadBackend *backend);
int input_pad_backend_init_genesis_6_button(InputPadBackend *backend);
int input_pad_backend_init_neogeo_4_button(InputPadBackend *backend);
int input_pad_backend_init_saturn_6_button(InputPadBackend *backend);
int input_pad_backend_init_dreamcast(InputPadBackend *backend);
int input_pad_backend_init_n64(InputPadBackend *backend);
int input_pad_backend_init_gamecube(InputPadBackend *backend);
int input_pad_backend_init_switch_pro(InputPadBackend *backend);
int input_pad_backend_init_joycon_pair(InputPadBackend *backend);
int input_pad_backend_init_wii_classic(InputPadBackend *backend);
int input_pad_backend_init_wii_u_pro(InputPadBackend *backend);
int input_pad_backend_init_playstation_digital(InputPadBackend *backend);
int input_pad_backend_init_dualshock(InputPadBackend *backend);
int input_pad_backend_init_psp(InputPadBackend *backend);
int input_pad_backend_init_psvita(InputPadBackend *backend);
int input_pad_backend_init_xinput(InputPadBackend *backend);
int input_pad_backend_init_xbox_elite(InputPadBackend *backend);
int input_pad_backend_init_arcade_2_button(InputPadBackend *backend);
int input_pad_backend_init_arcade_4_button(InputPadBackend *backend);
int input_pad_backend_init_arcade_6_button(InputPadBackend *backend);
int input_pad_backend_init_arcade_8_button(InputPadBackend *backend);

int input_pad_backend_set_snapshot(InputPadBackend *backend,
                                   const InputPadSnapshot *snapshot);
int input_pad_backend_enable_hat_to_dpad(InputPadBackend *backend, int enabled);
int input_pad_backend_enable_left_stick_to_dpad(InputPadBackend *backend, int enabled);
int input_pad_backend_enable_right_stick_to_cpad(InputPadBackend *backend, int enabled);
int input_pad_backend_enable_triggers_to_buttons(InputPadBackend *backend, int enabled);
int input_pad_backend_set_thresholds(InputPadBackend *backend,
                                     short stick_threshold,
                                     unsigned short trigger_threshold);

short input_pad_backend_left_x(const InputPadBackend *backend);
short input_pad_backend_left_y(const InputPadBackend *backend);
short input_pad_backend_right_x(const InputPadBackend *backend);
short input_pad_backend_right_y(const InputPadBackend *backend);
unsigned short input_pad_backend_left_trigger(const InputPadBackend *backend);
unsigned short input_pad_backend_right_trigger(const InputPadBackend *backend);

int input_pad_backend_poll(void *user_data, input_bits_t *out_bits);

#ifdef __cplusplus
}
#endif

#endif /* INPUT_BACKEND_PAD_H */
