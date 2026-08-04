#include "ecg_hud.h"
#include "ecg_fixed.h"
#include "ecg_font5x7.h"
#include "ecg_profiles.h"
#include "ecg_renderer.h"
#include "ecg_surface.h"

#include <string.h>

static int ecg_hud_ascii_upper(int ch)
{
    if (ch >= 'a' && ch <= 'z') {
        return ch - ('a' - 'A');
    }
    return ch;
}

static int ecg_hud_name_equals(const char *left, const char *right)
{
    int a;
    int b;

    if (left == (const char *)0 || right == (const char *)0) {
        return 0;
    }

    while (*left != '\0' && *right != '\0') {
        a = ecg_hud_ascii_upper((unsigned char)*left);
        b = ecg_hud_ascii_upper((unsigned char)*right);
        if (a != b) {
            return 0;
        }
        left++;
        right++;
    }

    return *left == '\0' && *right == '\0';
}

static ECG_Status ecg_hud_copy_name(char destination[ECG_HUD_STATE_NAME_CAPACITY],
                                    const char *source)
{
    unsigned int i;

    if (destination == (char *)0 || source == (const char *)0) {
        return ECG_STATUS_NULL;
    }
    if (source[0] == '\0') {
        return ECG_STATUS_BAD_ARGUMENT;
    }

    i = 0u;
    while (source[i] != '\0' && i + 1u < ECG_HUD_STATE_NAME_CAPACITY) {
        destination[i] = (char)ecg_hud_ascii_upper((unsigned char)source[i]);
        i++;
    }
    destination[i] = '\0';

    if (source[i] != '\0') {
        return ECG_STATUS_CAPACITY;
    }
    return ECG_STATUS_OK;
}

static void ecg_hud_box_default(ECG_HudBoxConfig *box)
{
    if (box == (ECG_HudBoxConfig *)0) {
        return;
    }

    box->x = -4;
    box->y = -4;
    box->width = 330u;
    box->height = 104u;
    box->filled = 0u;
    box->color = ecg_color_make(80u, 92u, 86u);
}

ECG_Status ecg_hud_config_find_state(const ECG_HudConfig *config,
                                     const char *name,
                                     ECG_HudState *state_out)
{
    unsigned int i;

    if (config == (const ECG_HudConfig *)0 ||
        name == (const char *)0 ||
        state_out == (ECG_HudState *)0) {
        return ECG_STATUS_NULL;
    }
    if (config->states == (ECG_HudStateDef *)0) {
        return ECG_STATUS_BAD_ARGUMENT;
    }

    for (i = 0u; i < config->state_count; i++) {
        if (ecg_hud_name_equals(name, config->states[i].name)) {
            *state_out = i;
            return ECG_STATUS_OK;
        }
    }

    return ECG_STATUS_NOT_FOUND;
}

ECG_Status ecg_hud_config_add_state(ECG_HudConfig *config,
                                    const char *name,
                                    ECG_Color color,
                                    ECG_Color gradient,
                                    ECG_Color glow_color,
                                    unsigned int profile_index,
                                    ECG_HudState *state_out)
{
    ECG_HudState existing;
    ECG_HudState index;
    ECG_Status status;

    if (config == (ECG_HudConfig *)0 || name == (const char *)0) {
        return ECG_STATUS_NULL;
    }
    if (config->states == (ECG_HudStateDef *)0 ||
        config->state_capacity == 0u) {
        return ECG_STATUS_BAD_ARGUMENT;
    }

    status = ecg_hud_config_find_state(config, name, &existing);
    if (status == ECG_STATUS_OK) {
        config->states[existing].color = color;
        config->states[existing].gradient = gradient;
        config->states[existing].glow_color = glow_color;
        config->states[existing].profile_index = profile_index;
        if (state_out != (ECG_HudState *)0) {
            *state_out = existing;
        }
        return ECG_STATUS_OK;
    }
    if (status != ECG_STATUS_NOT_FOUND) {
        return status;
    }
    if (config->state_count >= config->state_capacity) {
        return ECG_STATUS_CAPACITY;
    }

    index = config->state_count;
    status = ecg_hud_copy_name(config->states[index].name, name);
    if (status != ECG_STATUS_OK) {
        return status;
    }
    config->states[index].color = color;
    config->states[index].gradient = gradient;
    config->states[index].glow_color = glow_color;
    config->states[index].profile_index = profile_index;
    config->state_count++;

    if (state_out != (ECG_HudState *)0) {
        *state_out = index;
    }
    return ECG_STATUS_OK;
}

ECG_Status ecg_hud_config_bind_state_storage(ECG_HudConfig *config,
                                             ECG_HudStateDef *storage,
                                             unsigned int capacity,
                                             unsigned int copy_existing)
{
    unsigned int i;
    unsigned int old_count;
    ECG_HudStateDef *old_states;

    if (config == (ECG_HudConfig *)0 ||
        storage == (ECG_HudStateDef *)0) {
        return ECG_STATUS_NULL;
    }
    if (capacity == 0u) {
        return ECG_STATUS_BAD_ARGUMENT;
    }

    old_count = config->state_count;
    old_states = config->states;
    if (copy_existing != 0u && capacity < old_count) {
        return ECG_STATUS_CAPACITY;
    }

    if (copy_existing != 0u && old_states != (ECG_HudStateDef *)0) {
        for (i = 0u; i < old_count; i++) {
            storage[i] = old_states[i];
        }
    }

    config->states = storage;
    config->state_capacity = capacity;
    if (copy_existing != 0u) {
        config->state_count = old_count;
        if (config->state_count == 0u) {
            config->state = 0u;
        } else if (config->state >= config->state_count) {
            config->state = config->state_count - 1u;
        }
    } else {
        config->state_count = 0u;
        config->state = 0u;
    }

    return ECG_STATUS_OK;
}

ECG_Status ecg_hud_config_clear_states(ECG_HudConfig *config)
{
    if (config == (ECG_HudConfig *)0) {
        return ECG_STATUS_NULL;
    }
    if (config->states == (ECG_HudStateDef *)0) {
        return ECG_STATUS_BAD_ARGUMENT;
    }

    config->state_count = 0u;
    config->state = 0u;
    return ECG_STATUS_OK;
}

void ecg_hud_config_default(ECG_HudConfig *config_out)
{
    if (config_out == (ECG_HudConfig *)0) {
        return;
    }

    config_out->x = 8;
    config_out->y = 8;
    config_out->scale_x_q8 = ECG_FIXED_ONE;
    config_out->scale_y_q8 = ECG_FIXED_ONE;
    config_out->state = ECG_HUD_STATE_FINE;
    config_out->offset = 42u;
    config_out->visible_cols = ECG_VISIBLE_COLS_DEFAULT;
    config_out->draw_flags = ECG_HUD_DRAW_DEFAULT;

    config_out->active_x = 80;
    config_out->active_y = 0;
    config_out->overview_x = 340;
    config_out->overview_y = 0;

    config_out->state_text_x = 0;
    config_out->state_text_y = 20;
    config_out->state_text_scale = 2u;

    config_out->custom_text_storage[0] = '\0';
    config_out->custom_text = config_out->custom_text_storage;
    config_out->custom_text_x = 0;
    config_out->custom_text_y = -12;
    config_out->custom_text_scale = 1u;
    config_out->custom_text_color = ecg_color_make(220u, 235u, 225u);

    ecg_hud_box_default(&config_out->background_box);
    ecg_hud_box_default(&config_out->overlay_box);

    config_out->states = config_out->internal_states;
    config_out->state_count = 0u;
    config_out->state_capacity = ECG_HUD_INTERNAL_STATE_CAPACITY;

    (void)ecg_hud_config_add_state(config_out,
                                   "FINE",
                                   ecg_color_make(32u, 255u, 32u),
                                   ecg_color_make(1u, 8u, 1u),
                                   ecg_color_make(32u, 255u, 32u),
                                   ECG_PROFILE_FINE,
                                   (ECG_HudState *)0);
    (void)ecg_hud_config_add_state(config_out,
                                   "CAUTION",
                                   ecg_color_make(255u, 255u, 32u),
                                   ecg_color_make(8u, 8u, 1u),
                                   ecg_color_make(255u, 255u, 32u),
                                   ECG_PROFILE_CAUTION,
                                   (ECG_HudState *)0);
    (void)ecg_hud_config_add_state(config_out,
                                   "ORANGE",
                                   ecg_color_make(255u, 128u, 32u),
                                   ecg_color_make(8u, 4u, 1u),
                                   ecg_color_make(255u, 128u, 32u),
                                   ECG_PROFILE_ORANGE,
                                   (ECG_HudState *)0);
    (void)ecg_hud_config_add_state(config_out,
                                   "DANGER",
                                   ecg_color_make(255u, 32u, 32u),
                                   ecg_color_make(8u, 1u, 1u),
                                   ecg_color_make(255u, 32u, 32u),
                                   ECG_PROFILE_DANGER,
                                   (ECG_HudState *)0);
    (void)ecg_hud_config_add_state(config_out,
                                   "POISON",
                                   ecg_color_make(255u, 32u, 255u),
                                   ecg_color_make(8u, 1u, 8u),
                                   ecg_color_make(255u, 32u, 255u),
                                   ECG_PROFILE_POISON,
                                   (ECG_HudState *)0);

    ecg_render_config_default(&config_out->active_render);
    ecg_render_config_default(&config_out->overview_render);
}

const char *ecg_hud_state_name(ECG_HudState state)
{
    if (state >= ECG_HUD_DEFAULT_STATE_COUNT) {
        return "UNKNOWN";
    }
    return ecg_profile_name(state);
}

ECG_Status ecg_hud_state_from_name(const char *name,
                                   ECG_HudState *state_out)
{
    unsigned int i;

    if (name == (const char *)0 || state_out == (ECG_HudState *)0) {
        return ECG_STATUS_NULL;
    }

    for (i = 0u; i < ECG_HUD_DEFAULT_STATE_COUNT; i++) {
        if (ecg_hud_name_equals(name, ecg_profile_name(i))) {
            *state_out = i;
            return ECG_STATUS_OK;
        }
    }

    return ECG_STATUS_NOT_FOUND;
}

const char *ecg_hud_config_state_name(const ECG_HudConfig *config)
{
    if (config == (const ECG_HudConfig *)0 ||
        config->states == (ECG_HudStateDef *)0 ||
        config->state >= config->state_count) {
        return "UNKNOWN";
    }
    return config->states[config->state].name;
}

ECG_Status ecg_hud_config_set_state_name(ECG_HudConfig *config,
                                         const char *name)
{
    ECG_HudState state;
    ECG_Status status;

    if (config == (ECG_HudConfig *)0) {
        return ECG_STATUS_NULL;
    }

    status = ecg_hud_config_find_state(config, name, &state);
    if (status != ECG_STATUS_OK) {
        return status;
    }

    config->state = state;
    return ECG_STATUS_OK;
}

ECG_Status ecg_hud_config_set_custom_text(ECG_HudConfig *config,
                                          const char *text)
{
    unsigned int i;

    if (config == (ECG_HudConfig *)0 || text == (const char *)0) {
        return ECG_STATUS_NULL;
    }

    i = 0u;
    while (text[i] != '\0' && i + 1u < ECG_HUD_CUSTOM_TEXT_CAPACITY) {
        config->custom_text_storage[i] = text[i];
        i++;
    }
    config->custom_text_storage[i] = '\0';
    config->custom_text = config->custom_text_storage;

    return text[i] == '\0' ? ECG_STATUS_OK : ECG_STATUS_CAPACITY;
}

static ECG_Status ecg_hud_parse_int(const char *text, int *value_out)
{
    int sign;
    int value;
    int digit_count;

    if (text == (const char *)0 || value_out == (int *)0) {
        return ECG_STATUS_NULL;
    }

    sign = 1;
    value = 0;
    digit_count = 0;

    if (*text == '-') {
        sign = -1;
        text++;
    } else if (*text == '+') {
        text++;
    }

    while (*text >= '0' && *text <= '9') {
        value = value * 10 + (*text - '0');
        digit_count++;
        text++;
    }

    if (digit_count == 0 || *text != '\0') {
        return ECG_STATUS_BAD_ARGUMENT;
    }

    *value_out = value * sign;
    return ECG_STATUS_OK;
}

static ECG_Status ecg_hud_parse_uint(const char *text,
                                     unsigned int *value_out)
{
    int value;
    ECG_Status status;

    if (value_out == (unsigned int *)0) {
        return ECG_STATUS_NULL;
    }

    status = ecg_hud_parse_int(text, &value);
    if (status != ECG_STATUS_OK || value < 0) {
        return ECG_STATUS_BAD_ARGUMENT;
    }

    *value_out = (unsigned int)value;
    return ECG_STATUS_OK;
}

static ECG_Status ecg_hud_parse_bool(const char *text,
                                     unsigned int *value_out)
{
    if (text == (const char *)0 || value_out == (unsigned int *)0) {
        return ECG_STATUS_NULL;
    }

    if (ecg_hud_name_equals(text, "1") ||
        ecg_hud_name_equals(text, "TRUE") ||
        ecg_hud_name_equals(text, "YES") ||
        ecg_hud_name_equals(text, "ON")) {
        *value_out = 1u;
        return ECG_STATUS_OK;
    }

    if (ecg_hud_name_equals(text, "0") ||
        ecg_hud_name_equals(text, "FALSE") ||
        ecg_hud_name_equals(text, "NO") ||
        ecg_hud_name_equals(text, "OFF")) {
        *value_out = 0u;
        return ECG_STATUS_OK;
    }

    return ECG_STATUS_BAD_ARGUMENT;
}

static ECG_Status ecg_hud_parse_color(const char *text,
                                      ECG_Color *color_out)
{
    unsigned int values[3];
    unsigned int component;
    unsigned int value;
    unsigned int digit_count;

    if (text == (const char *)0 || color_out == (ECG_Color *)0) {
        return ECG_STATUS_NULL;
    }

    component = 0u;
    value = 0u;
    digit_count = 0u;

    while (1) {
        if (*text >= '0' && *text <= '9') {
            value = value * 10u + (unsigned int)(*text - '0');
            if (value > 255u) {
                return ECG_STATUS_BAD_ARGUMENT;
            }
            digit_count++;
            text++;
        } else if (*text == ',' || *text == '\0') {
            if (digit_count == 0u || component >= 3u) {
                return ECG_STATUS_BAD_ARGUMENT;
            }
            values[component] = value;
            component++;
            value = 0u;
            digit_count = 0u;
            if (*text == '\0') {
                break;
            }
            text++;
        } else {
            return ECG_STATUS_BAD_ARGUMENT;
        }
    }

    if (component != 3u) {
        return ECG_STATUS_BAD_ARGUMENT;
    }

    color_out->r = (ECG_U8)values[0];
    color_out->g = (ECG_U8)values[1];
    color_out->b = (ECG_U8)values[2];
    return ECG_STATUS_OK;
}

static ECG_Status ecg_hud_parse_fixed_positive(const char *text,
                                                ECG_FixedQ8 *value_out)
{
    unsigned long whole;
    unsigned long fraction;
    unsigned long denominator;
    unsigned long numerator;
    unsigned int digits;
    const char *cursor;
    const char *slash;
    int left;
    int right;
    ECG_Status status;

    if (text == (const char *)0 || value_out == (ECG_FixedQ8 *)0) {
        return ECG_STATUS_NULL;
    }

    slash = strchr(text, '/');
    if (slash != (const char *)0) {
        char left_text[24];
        char right_text[24];
        unsigned int left_len;
        unsigned int right_len;
        unsigned int i;

        left_len = (unsigned int)(slash - text);
        right_len = (unsigned int)strlen(slash + 1);
        if (left_len == 0u || right_len == 0u ||
            left_len >= sizeof(left_text) || right_len >= sizeof(right_text)) {
            return ECG_STATUS_BAD_ARGUMENT;
        }
        for (i = 0u; i < left_len; i++) {
            left_text[i] = text[i];
        }
        left_text[left_len] = '\0';
        for (i = 0u; i < right_len; i++) {
            right_text[i] = slash[1 + i];
        }
        right_text[right_len] = '\0';

        status = ecg_hud_parse_int(left_text, &left);
        if (status != ECG_STATUS_OK) {
            return status;
        }
        status = ecg_hud_parse_int(right_text, &right);
        if (status != ECG_STATUS_OK || left <= 0 || right <= 0) {
            return ECG_STATUS_BAD_ARGUMENT;
        }
        *value_out = ecg_fixed_from_ratio(left, right);
        return *value_out > (ECG_FixedQ8)0 ?
               ECG_STATUS_OK : ECG_STATUS_BAD_ARGUMENT;
    }

    whole = 0ul;
    fraction = 0ul;
    denominator = 1ul;
    digits = 0u;
    cursor = text;

    while (*cursor >= '0' && *cursor <= '9') {
        whole = whole * 10ul + (unsigned long)(*cursor - '0');
        cursor++;
    }

    if (*cursor == '.') {
        cursor++;
        while (*cursor >= '0' && *cursor <= '9' && digits < 6u) {
            fraction = fraction * 10ul + (unsigned long)(*cursor - '0');
            denominator *= 10ul;
            digits++;
            cursor++;
        }
    }

    if (*cursor != '\0' || (whole == 0ul && fraction == 0ul)) {
        return ECG_STATUS_BAD_ARGUMENT;
    }
    if (whole > 32767ul || denominator > 1000000ul) {
        return ECG_STATUS_BAD_ARGUMENT;
    }

    numerator = whole * denominator + fraction;
    if (numerator > 2147483647ul) {
        return ECG_STATUS_BAD_ARGUMENT;
    }
    *value_out = ecg_fixed_from_ratio((int)numerator, (int)denominator);
    return *value_out > (ECG_FixedQ8)0 ?
           ECG_STATUS_OK : ECG_STATUS_BAD_ARGUMENT;
}

static ECG_Status ecg_hud_parse_style(const char *text,
                                      unsigned int *style_out)
{
    if (text == (const char *)0 || style_out == (unsigned int *)0) {
        return ECG_STATUS_NULL;
    }

    if (ecg_hud_name_equals(text, "CONTINUOUS") ||
        ecg_hud_name_equals(text, "SOLID")) {
        *style_out = ECG_SIGNAL_STYLE_CONTINUOUS;
        return ECG_STATUS_OK;
    }
    if (ecg_hud_name_equals(text, "DOTTED") ||
        ecg_hud_name_equals(text, "DOTS")) {
        *style_out = ECG_SIGNAL_STYLE_DOTTED;
        return ECG_STATUS_OK;
    }
    if (ecg_hud_name_equals(text, "BARS") ||
        ecg_hud_name_equals(text, "COLUMNS")) {
        *style_out = ECG_SIGNAL_STYLE_BARS;
        return ECG_STATUS_OK;
    }

    return ECG_STATUS_BAD_ARGUMENT;
}

static void ecg_hud_set_flag(unsigned long *flags,
                             unsigned long mask,
                             unsigned int enabled)
{
    if (enabled != 0u) {
        *flags |= mask;
    } else {
        *flags &= ~mask;
    }
}

static ECG_Status ecg_hud_apply_show_flag(ECG_HudConfig *config,
                                          const char *key,
                                          const char *value)
{
    unsigned int enabled;
    unsigned long hud_mask;
    unsigned long render_mask;
    ECG_Status status;

    status = ecg_hud_parse_bool(value, &enabled);
    if (status != ECG_STATUS_OK) {
        return status;
    }

    hud_mask = 0ul;
    render_mask = 0ul;

    if (strcmp(key, "hud.show.active") == 0) {
        hud_mask = ECG_HUD_DRAW_ACTIVE;
    } else if (strcmp(key, "hud.show.overview") == 0) {
        hud_mask = ECG_HUD_DRAW_OVERVIEW;
    } else if (strcmp(key, "hud.show.state_text") == 0) {
        hud_mask = ECG_HUD_DRAW_STATE_TEXT;
    } else if (strcmp(key, "hud.show.custom_text") == 0) {
        hud_mask = ECG_HUD_DRAW_CUSTOM_TEXT;
    } else if (strcmp(key, "hud.show.background_box") == 0) {
        hud_mask = ECG_HUD_DRAW_BACKGROUND_BOX;
    } else if (strcmp(key, "hud.show.overlay_box") == 0) {
        hud_mask = ECG_HUD_DRAW_OVERLAY_BOX;
    } else if (strcmp(key, "hud.show.grid") == 0) {
        render_mask = ECG_RENDER_DRAW_GRID;
    } else if (strcmp(key, "hud.show.axis") == 0) {
        render_mask = ECG_RENDER_DRAW_AXIS;
    } else if (strcmp(key, "hud.show.frame") == 0) {
        render_mask = ECG_RENDER_DRAW_FRAME;
    } else if (strcmp(key, "hud.show.signal") == 0) {
        render_mask = ECG_RENDER_DRAW_SIGNAL;
    } else if (strcmp(key, "hud.show.window") == 0) {
        render_mask = ECG_RENDER_DRAW_WINDOW;
    } else {
        return ECG_STATUS_BAD_ARGUMENT;
    }

    if (hud_mask != 0ul) {
        ecg_hud_set_flag(&config->draw_flags, hud_mask, enabled);
    }
    if (render_mask != 0ul) {
        ecg_hud_set_flag(&config->active_render.draw_flags,
                         render_mask,
                         enabled);
        ecg_hud_set_flag(&config->overview_render.draw_flags,
                         render_mask,
                         enabled);
    }

    return ECG_STATUS_OK;
}

static ECG_Status ecg_hud_find_or_add_state(ECG_HudConfig *config,
                                            const char *name,
                                            ECG_HudState *state_out)
{
    ECG_Status status;

    status = ecg_hud_config_find_state(config, name, state_out);
    if (status == ECG_STATUS_OK) {
        return status;
    }
    if (status != ECG_STATUS_NOT_FOUND) {
        return status;
    }

    return ecg_hud_config_add_state(config,
                                    name,
                                    ecg_color_make(255u, 255u, 255u),
                                    ecg_color_make(4u, 4u, 4u),
                                    ecg_color_make(255u, 255u, 255u),
                                    ECG_PROFILE_FINE,
                                    state_out);
}

static ECG_Status ecg_hud_apply_named_state_color(ECG_HudConfig *config,
                                                   const char *state_name,
                                                   const char *value,
                                                   unsigned int field)
{
    ECG_HudState state;
    ECG_Color color;
    ECG_Status status;

    status = ecg_hud_find_or_add_state(config, state_name, &state);
    if (status != ECG_STATUS_OK) {
        return status;
    }
    status = ecg_hud_parse_color(value, &color);
    if (status != ECG_STATUS_OK) {
        return status;
    }

    if (field == 0u) {
        config->states[state].color = color;
    } else if (field == 1u) {
        config->states[state].gradient = color;
    } else {
        config->states[state].glow_color = color;
    }
    return ECG_STATUS_OK;
}

static void ecg_hud_apply_render_style(ECG_HudConfig *config,
                                       unsigned int style)
{
    config->active_render.signal_style = style;
    config->overview_render.signal_style = style;
}

static void ecg_hud_apply_render_uint(ECG_HudConfig *config,
                                      const char *key,
                                      unsigned int value)
{
    if (strcmp(key, "hud.signal.line_width") == 0) {
        config->active_render.bar_width_px = value;
        config->overview_render.bar_width_px = value;
    } else if (strcmp(key, "hud.signal.dot_on") == 0) {
        config->active_render.dot_on_px = value;
        config->overview_render.dot_on_px = value;
    } else if (strcmp(key, "hud.signal.dot_off") == 0) {
        config->active_render.dot_off_px = value;
        config->overview_render.dot_off_px = value;
    } else if (strcmp(key, "hud.signal.glow_radius") == 0) {
        config->active_render.glow_radius_px = value;
        config->overview_render.glow_radius_px = value;
    } else if (strcmp(key, "hud.signal.glow_intensity") == 0) {
        config->active_render.glow_intensity = value;
        config->overview_render.glow_intensity = value;
    }
}

ECG_Status ecg_hud_config_apply_pair(ECG_HudConfig *config,
                                     const char *key,
                                     const char *value)
{
    int int_value;
    unsigned int uint_value;
    unsigned int bool_value;
    unsigned int style;
    ECG_HudState state;
    ECG_FixedQ8 fixed_value;
    ECG_Color color;
    ECG_Status status;
    const char *state_name;

    if (config == (ECG_HudConfig *)0 ||
        key == (const char *)0 || value == (const char *)0) {
        return ECG_STATUS_NULL;
    }

    if (strcmp(key, "hud.state") == 0) {
        return ecg_hud_config_set_state_name(config, value);
    }
    if (strcmp(key, "hud.state.add") == 0) {
        return ecg_hud_find_or_add_state(config, value, &state);
    }
    if (strcmp(key, "hud.states.clear") == 0) {
        status = ecg_hud_parse_bool(value, &bool_value);
        if (status != ECG_STATUS_OK) {
            return status;
        }
        return bool_value != 0u ?
               ecg_hud_config_clear_states(config) : ECG_STATUS_OK;
    }
    if (strcmp(key, "hud.custom_text") == 0) {
        return ecg_hud_config_set_custom_text(config, value);
    }
    if (strncmp(key, "hud.show.", 9u) == 0) {
        return ecg_hud_apply_show_flag(config, key, value);
    }

    if (strncmp(key, "hud.state.", 10u) == 0) {
        state_name = key + 10;
        return ecg_hud_apply_named_state_color(config,
                                                state_name,
                                                value,
                                                0u);
    }
    if (strncmp(key, "hud.color.", 10u) == 0) {
        state_name = key + 10;
        return ecg_hud_apply_named_state_color(config,
                                                state_name,
                                                value,
                                                0u);
    }
    if (strncmp(key, "hud.gradient.", 13u) == 0) {
        state_name = key + 13;
        return ecg_hud_apply_named_state_color(config,
                                                state_name,
                                                value,
                                                1u);
    }
    if (strncmp(key, "hud.glow_color.", 15u) == 0) {
        state_name = key + 15;
        return ecg_hud_apply_named_state_color(config,
                                                state_name,
                                                value,
                                                2u);
    }
    if (strncmp(key, "hud.profile.", 12u) == 0) {
        state_name = key + 12;
        status = ecg_hud_find_or_add_state(config, state_name, &state);
        if (status != ECG_STATUS_OK) {
            return status;
        }
        status = ecg_hud_parse_uint(value, &uint_value);
        if (status != ECG_STATUS_OK) {
            return status;
        }
        config->states[state].profile_index = uint_value;
        return ECG_STATUS_OK;
    }

    if (strcmp(key, "hud.color") == 0 ||
        strcmp(key, "hud.gradient") == 0 ||
        strcmp(key, "hud.glow_color") == 0) {
        if (config->state >= config->state_count) {
            return ECG_STATUS_BAD_ARGUMENT;
        }
        status = ecg_hud_parse_color(value, &color);
        if (status != ECG_STATUS_OK) {
            return status;
        }
        if (strcmp(key, "hud.color") == 0) {
            config->states[config->state].color = color;
        } else if (strcmp(key, "hud.gradient") == 0) {
            config->states[config->state].gradient = color;
        } else {
            config->states[config->state].glow_color = color;
        }
        return ECG_STATUS_OK;
    }

    if (strcmp(key, "hud.signal.style") == 0) {
        status = ecg_hud_parse_style(value, &style);
        if (status != ECG_STATUS_OK) {
            return status;
        }
        ecg_hud_apply_render_style(config, style);
        return ECG_STATUS_OK;
    }
    if (strcmp(key, "hud.signal.dotted") == 0) {
        status = ecg_hud_parse_bool(value, &bool_value);
        if (status != ECG_STATUS_OK) {
            return status;
        }
        ecg_hud_apply_render_style(config,
            bool_value != 0u ? ECG_SIGNAL_STYLE_DOTTED :
                               ECG_SIGNAL_STYLE_CONTINUOUS);
        return ECG_STATUS_OK;
    }
    if (strcmp(key, "hud.signal.glow") == 0) {
        status = ecg_hud_parse_bool(value, &bool_value);
        if (status != ECG_STATUS_OK) {
            return status;
        }
        config->active_render.glow_enabled = bool_value;
        config->overview_render.glow_enabled = bool_value;
        return ECG_STATUS_OK;
    }
    if (strcmp(key, "hud.signal.line_width") == 0 ||
        strcmp(key, "hud.signal.dot_on") == 0 ||
        strcmp(key, "hud.signal.dot_off") == 0 ||
        strcmp(key, "hud.signal.glow_radius") == 0 ||
        strcmp(key, "hud.signal.glow_intensity") == 0) {
        status = ecg_hud_parse_uint(value, &uint_value);
        if (status != ECG_STATUS_OK) {
            return status;
        }
        if ((strcmp(key, "hud.signal.glow_intensity") == 0 &&
             uint_value > 255u) ||
            (strcmp(key, "hud.signal.line_width") == 0 &&
             uint_value == 0u)) {
            return ECG_STATUS_BAD_ARGUMENT;
        }
        ecg_hud_apply_render_uint(config, key, uint_value);
        return ECG_STATUS_OK;
    }

    if (strcmp(key, "hud.scale") == 0 ||
        strcmp(key, "hud.scale.x") == 0 ||
        strcmp(key, "hud.scale.y") == 0 ||
        strcmp(key, "hud.active.scale.x") == 0 ||
        strcmp(key, "hud.active.scale.y") == 0 ||
        strcmp(key, "hud.overview.scale.x") == 0 ||
        strcmp(key, "hud.overview.scale.y") == 0) {
        status = ecg_hud_parse_fixed_positive(value, &fixed_value);
        if (status != ECG_STATUS_OK) {
            return status;
        }

        if (strcmp(key, "hud.scale") == 0) {
            config->scale_x_q8 = fixed_value;
            config->scale_y_q8 = fixed_value;
        } else if (strcmp(key, "hud.scale.x") == 0) {
            config->scale_x_q8 = fixed_value;
        } else if (strcmp(key, "hud.scale.y") == 0) {
            config->scale_y_q8 = fixed_value;
        } else if (strcmp(key, "hud.active.scale.x") == 0) {
            config->active_render.x_step_q8 = fixed_value;
        } else if (strcmp(key, "hud.active.scale.y") == 0) {
            config->active_render.y_step_q8 = fixed_value;
        } else if (strcmp(key, "hud.overview.scale.x") == 0) {
            config->overview_render.x_step_q8 = fixed_value;
        } else {
            config->overview_render.y_step_q8 = fixed_value;
        }
        return ECG_STATUS_OK;
    }

    if (strcmp(key, "hud.grid.color") == 0 ||
        strcmp(key, "hud.axis.color") == 0 ||
        strcmp(key, "hud.frame.color") == 0 ||
        strcmp(key, "hud.window.color") == 0 ||
        strcmp(key, "hud.custom_text.color") == 0 ||
        strcmp(key, "hud.background_box.color") == 0 ||
        strcmp(key, "hud.overlay_box.color") == 0) {
        status = ecg_hud_parse_color(value, &color);
        if (status != ECG_STATUS_OK) {
            return status;
        }

        if (strcmp(key, "hud.grid.color") == 0) {
            config->active_render.grid_color = color;
            config->overview_render.grid_color = color;
        } else if (strcmp(key, "hud.axis.color") == 0) {
            config->active_render.axis_color = color;
            config->overview_render.axis_color = color;
        } else if (strcmp(key, "hud.frame.color") == 0) {
            config->active_render.frame_color = color;
            config->overview_render.frame_color = color;
        } else if (strcmp(key, "hud.window.color") == 0) {
            config->active_render.window_color = color;
            config->overview_render.window_color = color;
        } else if (strcmp(key, "hud.custom_text.color") == 0) {
            config->custom_text_color = color;
        } else if (strcmp(key, "hud.background_box.color") == 0) {
            config->background_box.color = color;
        } else {
            config->overlay_box.color = color;
        }
        return ECG_STATUS_OK;
    }

    if (strcmp(key, "hud.x") == 0 ||
        strcmp(key, "hud.y") == 0 ||
        strcmp(key, "hud.active.x") == 0 ||
        strcmp(key, "hud.active.y") == 0 ||
        strcmp(key, "hud.overview.x") == 0 ||
        strcmp(key, "hud.overview.y") == 0 ||
        strcmp(key, "hud.state_text.x") == 0 ||
        strcmp(key, "hud.state_text.y") == 0 ||
        strcmp(key, "hud.custom_text.x") == 0 ||
        strcmp(key, "hud.custom_text.y") == 0) {
        status = ecg_hud_parse_int(value, &int_value);
        if (status != ECG_STATUS_OK) {
            return status;
        }

        if (strcmp(key, "hud.x") == 0) {
            config->x = int_value;
        } else if (strcmp(key, "hud.y") == 0) {
            config->y = int_value;
        } else if (strcmp(key, "hud.active.x") == 0) {
            config->active_x = int_value;
        } else if (strcmp(key, "hud.active.y") == 0) {
            config->active_y = int_value;
        } else if (strcmp(key, "hud.overview.x") == 0) {
            config->overview_x = int_value;
        } else if (strcmp(key, "hud.overview.y") == 0) {
            config->overview_y = int_value;
        } else if (strcmp(key, "hud.state_text.x") == 0) {
            config->state_text_x = int_value;
        } else if (strcmp(key, "hud.state_text.y") == 0) {
            config->state_text_y = int_value;
        } else if (strcmp(key, "hud.custom_text.x") == 0) {
            config->custom_text_x = int_value;
        } else {
            config->custom_text_y = int_value;
        }
        return ECG_STATUS_OK;
    }

    if (strcmp(key, "hud.offset") == 0 ||
        strcmp(key, "hud.visible_cols") == 0 ||
        strcmp(key, "hud.state_text.scale") == 0 ||
        strcmp(key, "hud.custom_text.scale") == 0) {
        status = ecg_hud_parse_uint(value, &uint_value);
        if (status != ECG_STATUS_OK) {
            return status;
        }
        if (strcmp(key, "hud.offset") != 0 && uint_value == 0u) {
            return ECG_STATUS_BAD_ARGUMENT;
        }

        if (strcmp(key, "hud.offset") == 0) {
            config->offset = uint_value;
        } else if (strcmp(key, "hud.visible_cols") == 0) {
            config->visible_cols = uint_value;
        } else if (strcmp(key, "hud.state_text.scale") == 0) {
            config->state_text_scale = uint_value;
        } else {
            config->custom_text_scale = uint_value;
        }
        return ECG_STATUS_OK;
    }

    return ECG_STATUS_BAD_ARGUMENT;
}

static int ecg_hud_scaled_position(int value, ECG_FixedQ8 scale_q8)
{
    return ecg_fixed_mul_int_to_int_floor(value, scale_q8);
}

static unsigned int ecg_hud_scaled_size(unsigned int value,
                                        ECG_FixedQ8 scale_q8)
{
    int scaled;

    scaled = ecg_fixed_mul_uint_to_int_ceil(value, scale_q8);
    return scaled > 0 ? (unsigned int)scaled : 1u;
}

static void ecg_hud_draw_box(ECG_Surface *surface,
                             int hud_x,
                             int hud_y,
                             const ECG_HudBoxConfig *box,
                             ECG_FixedQ8 scale_x_q8,
                             ECG_FixedQ8 scale_y_q8)
{
    int x;
    int y;
    unsigned int width;
    unsigned int height;

    if (box == (const ECG_HudBoxConfig *)0 ||
        box->width == 0u || box->height == 0u) {
        return;
    }

    x = hud_x + ecg_hud_scaled_position(box->x, scale_x_q8);
    y = hud_y + ecg_hud_scaled_position(box->y, scale_y_q8);
    width = ecg_hud_scaled_size(box->width, scale_x_q8);
    height = ecg_hud_scaled_size(box->height, scale_y_q8);

    if (box->filled != 0u) {
        (void)ecg_fill_rect(surface, x, y, width, height, box->color);
    } else {
        (void)ecg_draw_rect_outline(surface,
                                    x,
                                    y,
                                    width,
                                    height,
                                    box->color);
    }
}

ECG_Status ecg_draw_hud_ex(ECG_Surface *surface,
                           const ECG_Profile *profiles,
                           unsigned int profile_count,
                           const ECG_HudConfig *config)
{
    ECG_HudConfig local;
    const ECG_HudConfig *cfg;
    const ECG_HudStateDef *state_def;
    ECG_Profile profile;
    ECG_RenderConfig active_render;
    ECG_RenderConfig overview_render;
    ECG_FixedQ8 text_scale_x;
    ECG_FixedQ8 text_scale_y;
    int child_x;
    int child_y;

    if (!ecg_surface_is_valid(surface) ||
        profiles == (const ECG_Profile *)0) {
        return ECG_STATUS_NULL;
    }
    if (profile_count == 0u) {
        return ECG_STATUS_BAD_ARGUMENT;
    }

    if (config == (const ECG_HudConfig *)0) {
        ecg_hud_config_default(&local);
        cfg = &local;
    } else {
        cfg = config;
    }

    if (cfg->states == (ECG_HudStateDef *)0 ||
        cfg->state_count == 0u ||
        cfg->state >= cfg->state_count ||
        cfg->visible_cols == 0u ||
        cfg->scale_x_q8 <= (ECG_FixedQ8)0 ||
        cfg->scale_y_q8 <= (ECG_FixedQ8)0) {
        return ECG_STATUS_BAD_ARGUMENT;
    }

    state_def = &cfg->states[cfg->state];
    if (state_def->profile_index >= profile_count) {
        return ECG_STATUS_BAD_ARGUMENT;
    }

    profile = profiles[state_def->profile_index];
    profile.name = state_def->name;
    profile.color = state_def->color;
    profile.gradient = state_def->gradient;

    active_render = cfg->active_render;
    overview_render = cfg->overview_render;
    active_render.x_step_q8 = ecg_fixed_mul_q8(active_render.x_step_q8,
                                                cfg->scale_x_q8);
    active_render.y_step_q8 = ecg_fixed_mul_q8(active_render.y_step_q8,
                                                cfg->scale_y_q8);
    overview_render.x_step_q8 = ecg_fixed_mul_q8(overview_render.x_step_q8,
                                                  cfg->scale_x_q8);
    overview_render.y_step_q8 = ecg_fixed_mul_q8(overview_render.y_step_q8,
                                                  cfg->scale_y_q8);
    active_render.glow_color = state_def->glow_color;
    overview_render.glow_color = state_def->glow_color;
    active_render.use_profile_glow_color = 0u;
    overview_render.use_profile_glow_color = 0u;

    if ((cfg->draw_flags & ECG_HUD_DRAW_BACKGROUND_BOX) != 0ul) {
        ecg_hud_draw_box(surface,
                         cfg->x,
                         cfg->y,
                         &cfg->background_box,
                         cfg->scale_x_q8,
                         cfg->scale_y_q8);
    }

    if ((cfg->draw_flags & ECG_HUD_DRAW_STATE_TEXT) != 0ul) {
        text_scale_x = ecg_fixed_mul_q8(
                            ecg_fixed_from_int((int)cfg->state_text_scale),
                            cfg->scale_x_q8);
        text_scale_y = ecg_fixed_mul_q8(
                            ecg_fixed_from_int((int)cfg->state_text_scale),
                            cfg->scale_y_q8);
        child_x = cfg->x + ecg_hud_scaled_position(cfg->state_text_x,
                                                   cfg->scale_x_q8);
        child_y = cfg->y + ecg_hud_scaled_position(cfg->state_text_y,
                                                   cfg->scale_y_q8);
        (void)ecg_draw_text5x7_scaled_q8(surface,
                                         child_x,
                                         child_y,
                                         profile.name,
                                         profile.color,
                                         text_scale_x,
                                         text_scale_y);
    }

    if ((cfg->draw_flags & ECG_HUD_DRAW_CUSTOM_TEXT) != 0ul &&
        cfg->custom_text != (const char *)0) {
        text_scale_x = ecg_fixed_mul_q8(
                            ecg_fixed_from_int((int)cfg->custom_text_scale),
                            cfg->scale_x_q8);
        text_scale_y = ecg_fixed_mul_q8(
                            ecg_fixed_from_int((int)cfg->custom_text_scale),
                            cfg->scale_y_q8);
        child_x = cfg->x + ecg_hud_scaled_position(cfg->custom_text_x,
                                                   cfg->scale_x_q8);
        child_y = cfg->y + ecg_hud_scaled_position(cfg->custom_text_y,
                                                   cfg->scale_y_q8);
        (void)ecg_draw_text5x7_scaled_q8(surface,
                                         child_x,
                                         child_y,
                                         cfg->custom_text,
                                         cfg->custom_text_color,
                                         text_scale_x,
                                         text_scale_y);
    }

    if ((cfg->draw_flags & ECG_HUD_DRAW_ACTIVE) != 0ul) {
        child_x = cfg->x + ecg_hud_scaled_position(cfg->active_x,
                                                   cfg->scale_x_q8);
        child_y = cfg->y + ecg_hud_scaled_position(cfg->active_y,
                                                   cfg->scale_y_q8);
        (void)ecg_draw_active_window(surface,
                                     &profile,
                                     child_x,
                                     child_y,
                                     cfg->offset,
                                     cfg->visible_cols,
                                     &active_render);
    }

    if ((cfg->draw_flags & ECG_HUD_DRAW_OVERVIEW) != 0ul) {
        child_x = cfg->x + ecg_hud_scaled_position(cfg->overview_x,
                                                   cfg->scale_x_q8);
        child_y = cfg->y + ecg_hud_scaled_position(cfg->overview_y,
                                                   cfg->scale_y_q8);
        (void)ecg_draw_overview(surface,
                                &profile,
                                child_x,
                                child_y,
                                cfg->offset,
                                cfg->visible_cols,
                                &overview_render);
    }

    if ((cfg->draw_flags & ECG_HUD_DRAW_OVERLAY_BOX) != 0ul) {
        ecg_hud_draw_box(surface,
                         cfg->x,
                         cfg->y,
                         &cfg->overlay_box,
                         cfg->scale_x_q8,
                         cfg->scale_y_q8);
    }

    return ECG_STATUS_OK;
}

ECG_Status ecg_draw_hud(ECG_Surface *surface,
                        const ECG_Profile profiles[ECG_PROFILE_COUNT],
                        const ECG_HudConfig *config)
{
    return ecg_draw_hud_ex(surface,
                           profiles,
                           ECG_PROFILE_COUNT,
                           config);
}
