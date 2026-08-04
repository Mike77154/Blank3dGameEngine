#ifndef ECG_HUD_H
#define ECG_HUD_H

#include "ecg_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ECG_HUD_DRAW_ACTIVE          0x0001ul
#define ECG_HUD_DRAW_OVERVIEW        0x0002ul
#define ECG_HUD_DRAW_STATE_TEXT      0x0004ul
#define ECG_HUD_DRAW_CUSTOM_TEXT     0x0008ul
#define ECG_HUD_DRAW_BACKGROUND_BOX  0x0010ul
#define ECG_HUD_DRAW_OVERLAY_BOX     0x0020ul

#define ECG_HUD_DRAW_DEFAULT \
    (ECG_HUD_DRAW_ACTIVE | ECG_HUD_DRAW_STATE_TEXT)

#define ECG_HUD_CUSTOM_TEXT_CAPACITY 64u
#define ECG_HUD_STATE_NAME_CAPACITY 32u
#define ECG_HUD_INTERNAL_STATE_CAPACITY 16u
#define ECG_HUD_DEFAULT_STATE_COUNT 5u

#define ECG_HUD_STATE_FINE 0u
#define ECG_HUD_STATE_CAUTION 1u
#define ECG_HUD_STATE_ORANGE 2u
#define ECG_HUD_STATE_DANGER 3u
#define ECG_HUD_STATE_POISON 4u
#define ECG_HUD_STATE_COUNT ECG_HUD_DEFAULT_STATE_COUNT

typedef unsigned int ECG_HudState;

typedef struct ECG_HudStateDefTag {
    char name[ECG_HUD_STATE_NAME_CAPACITY];
    ECG_Color color;
    ECG_Color gradient;
    ECG_Color glow_color;
    unsigned int profile_index;
} ECG_HudStateDef;

typedef struct ECG_HudBoxConfigTag {
    int x;
    int y;
    unsigned int width;
    unsigned int height;
    unsigned int filled;
    ECG_Color color;
} ECG_HudBoxConfig;

typedef struct ECG_HudConfigTag {
    int x;
    int y;
    ECG_FixedQ8 scale_x_q8;
    ECG_FixedQ8 scale_y_q8;

    ECG_HudState state;
    unsigned int offset;
    unsigned int visible_cols;
    unsigned long draw_flags;

    int active_x;
    int active_y;
    int overview_x;
    int overview_y;

    int state_text_x;
    int state_text_y;
    unsigned int state_text_scale;

    const char *custom_text;
    char custom_text_storage[ECG_HUD_CUSTOM_TEXT_CAPACITY];
    int custom_text_x;
    int custom_text_y;
    unsigned int custom_text_scale;
    ECG_Color custom_text_color;

    ECG_HudBoxConfig background_box;
    ECG_HudBoxConfig overlay_box;

    ECG_HudStateDef internal_states[ECG_HUD_INTERNAL_STATE_CAPACITY];
    ECG_HudStateDef *states;
    unsigned int state_count;
    unsigned int state_capacity;

    ECG_RenderConfig active_render;
    ECG_RenderConfig overview_render;
} ECG_HudConfig;

void ecg_hud_config_default(ECG_HudConfig *config_out);

const char *ecg_hud_state_name(ECG_HudState state);
ECG_Status ecg_hud_state_from_name(const char *name,
                                   ECG_HudState *state_out);
const char *ecg_hud_config_state_name(const ECG_HudConfig *config);
ECG_Status ecg_hud_config_find_state(const ECG_HudConfig *config,
                                     const char *name,
                                     ECG_HudState *state_out);
ECG_Status ecg_hud_config_set_state_name(ECG_HudConfig *config,
                                         const char *name);
ECG_Status ecg_hud_config_bind_state_storage(ECG_HudConfig *config,
                                             ECG_HudStateDef *storage,
                                             unsigned int capacity,
                                             unsigned int copy_existing);
ECG_Status ecg_hud_config_clear_states(ECG_HudConfig *config);
ECG_Status ecg_hud_config_add_state(ECG_HudConfig *config,
                                    const char *name,
                                    ECG_Color color,
                                    ECG_Color gradient,
                                    ECG_Color glow_color,
                                    unsigned int profile_index,
                                    ECG_HudState *state_out);
ECG_Status ecg_hud_config_set_custom_text(ECG_HudConfig *config,
                                          const char *text);
ECG_Status ecg_hud_config_apply_pair(ECG_HudConfig *config,
                                     const char *key,
                                     const char *value);

ECG_Status ecg_draw_hud_ex(ECG_Surface *surface,
                           const ECG_Profile *profiles,
                           unsigned int profile_count,
                           const ECG_HudConfig *config);
ECG_Status ecg_draw_hud(ECG_Surface *surface,
                        const ECG_Profile profiles[ECG_PROFILE_COUNT],
                        const ECG_HudConfig *config);

#ifdef __cplusplus
}
#endif

#endif
