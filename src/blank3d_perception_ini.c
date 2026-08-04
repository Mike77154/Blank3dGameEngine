#include "blank3d_perception_ini.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define B3D_PERCEPTION_INI_LINE_CAP 384

static void b3d_perception_ini_status(char *status, size_t capacity,
                                      const char *text)
{
    if (!status || capacity == 0U) return;
    if (!text) text = "";
    strncpy(status, text, capacity - 1U);
    status[capacity - 1U] = '\0';
}

static char *b3d_perception_ini_trim(char *text)
{
    char *end;
    if (!text) return text;
    while (*text && isspace((unsigned char)*text)) ++text;
    end = text + strlen(text);
    while (end > text && isspace((unsigned char)end[-1])) --end;
    *end = '\0';
    return text;
}

static int b3d_perception_ini_equal(const char *a, const char *b)
{
    unsigned char ca;
    unsigned char cb;
    if (!a || !b) return 0;
    while (*a && *b) {
        ca = (unsigned char)tolower((unsigned char)*a++);
        cb = (unsigned char)tolower((unsigned char)*b++);
        if (ca != cb) return 0;
    }
    return *a == '\0' && *b == '\0';
}

static int b3d_perception_ini_bool(const char *text, int fallback)
{
    if (!text) return fallback;
    if (b3d_perception_ini_equal(text, "1") ||
        b3d_perception_ini_equal(text, "true") ||
        b3d_perception_ini_equal(text, "yes") ||
        b3d_perception_ini_equal(text, "on")) return 1;
    if (b3d_perception_ini_equal(text, "0") ||
        b3d_perception_ini_equal(text, "false") ||
        b3d_perception_ini_equal(text, "no") ||
        b3d_perception_ini_equal(text, "off")) return 0;
    return fallback;
}

static int b3d_perception_ini_int(const char *text, int fallback)
{
    char *end;
    long value;
    if (!text || !*text) return fallback;
    value = strtol(text, &end, 10);
    end = b3d_perception_ini_trim(end);
    if (*end != '\0') return fallback;
    if (value < -2147483647L) value = -2147483647L;
    if (value > 2147483647L) value = 2147483647L;
    return (int)value;
}

static g3d_fix b3d_perception_ini_fixed(const char *text, g3d_fix fallback)
{
    long sign;
    long whole;
    long fraction;
    long divisor;
    const char *p;
    int digits;
    if (!text || !*text) return fallback;
    p = text;
    sign = 1L;
    if (*p == '-') {
        sign = -1L;
        ++p;
    } else if (*p == '+') {
        ++p;
    }
    if (!isdigit((unsigned char)*p) && *p != '.') return fallback;
    whole = 0L;
    while (isdigit((unsigned char)*p)) {
        whole = whole * 10L + (long)(*p - '0');
        ++p;
    }
    fraction = 0L;
    divisor = 1L;
    digits = 0;
    if (*p == '.') {
        ++p;
        while (isdigit((unsigned char)*p) && digits < 5) {
            fraction = fraction * 10L + (long)(*p - '0');
            divisor *= 10L;
            ++digits;
            ++p;
        }
        while (isdigit((unsigned char)*p)) ++p;
    }
    while (*p && isspace((unsigned char)*p)) ++p;
    if (*p != '\0') return fallback;
    return (g3d_fix)(sign * (whole * (long)G3D_FIX_ONE +
        (fraction * (long)G3D_FIX_ONE) / divisor));
}

static int b3d_perception_ini_is_section(const char *section,
                                         const char *name)
{
    return b3d_perception_ini_equal(section, name);
}

static int b3d_perception_ini_apply_truth(const char *key,
                                          const char *value,
                                          Blank3DTruthGate *truth_gate,
                                          g3d_fix *truth_range)
{
    if (b3d_perception_ini_equal(key, "profile") ||
        b3d_perception_ini_equal(key, "truth_profile") ||
        b3d_perception_ini_equal(key, "truthprofile")) {
        if (truth_gate)
            (void)blank3d_truth_gate_set_profile_name(truth_gate, value);
        return 1;
    }
    if (b3d_perception_ini_equal(key, "range") ||
        b3d_perception_ini_equal(key, "truth_range") ||
        b3d_perception_ini_equal(key, "truthrange")) {
        if (truth_range)
            *truth_range = b3d_perception_ini_fixed(value, *truth_range);
        return 1;
    }
    return 0;
}

static int b3d_perception_ini_apply_eyes(const char *key,
                                         const char *value,
                                         Blank3DPerceptionConfig *config)
{
    Blank3DPerceptionAgent temporary;
    if (!config) return 0;
    if (b3d_perception_ini_equal(key, "shape") ||
        b3d_perception_ini_equal(key, "eye_shape") ||
        b3d_perception_ini_equal(key, "eyeshape") ||
        b3d_perception_ini_equal(key, "vision_shape") ||
        b3d_perception_ini_equal(key, "visionshape")) {
        memset(&temporary, 0, sizeof(temporary));
        temporary.config = *config;
        if (blank3d_perception_set_eye_shape(&temporary, value))
            config->eye_shape = temporary.config.eye_shape;
        return 1;
    }
    if (b3d_perception_ini_equal(key, "range") ||
        b3d_perception_ini_equal(key, "view_range") ||
        b3d_perception_ini_equal(key, "viewrange") ||
        b3d_perception_ini_equal(key, "perception_range") ||
        b3d_perception_ini_equal(key, "perceptionrange")) {
        config->view_range = b3d_perception_ini_fixed(value,
                                                       config->view_range);
        return 1;
    }
    if (b3d_perception_ini_equal(key, "horizontal_fov") ||
        b3d_perception_ini_equal(key, "horizontalfov") ||
        b3d_perception_ini_equal(key, "view_fov") ||
        b3d_perception_ini_equal(key, "viewfov") ||
        b3d_perception_ini_equal(key, "fov")) {
        config->horizontal_fov_degrees = b3d_perception_ini_int(
            value, config->horizontal_fov_degrees);
        return 1;
    }
    if (b3d_perception_ini_equal(key, "vertical_fov") ||
        b3d_perception_ini_equal(key, "verticalfov")) {
        config->vertical_fov_degrees = b3d_perception_ini_int(
            value, config->vertical_fov_degrees);
        return 1;
    }
    if (b3d_perception_ini_equal(key, "require_line_of_sight") ||
        b3d_perception_ini_equal(key, "require_los") ||
        b3d_perception_ini_equal(key, "requirelos") ||
        b3d_perception_ini_equal(key, "eyes_require_los") ||
        b3d_perception_ini_equal(key, "eyesrequirelos")) {
        config->require_line_of_sight = b3d_perception_ini_bool(
            value, config->require_line_of_sight);
        return 1;
    }
    return 0;
}

static int b3d_perception_ini_apply_hearing(const char *key,
                                            const char *value,
                                            Blank3DPerceptionConfig *config)
{
    if (!config) return 0;
    if (b3d_perception_ini_equal(key, "range") ||
        b3d_perception_ini_equal(key, "hearing_range") ||
        b3d_perception_ini_equal(key, "hearingrange")) {
        config->hearing_range = b3d_perception_ini_fixed(
            value, config->hearing_range);
        return 1;
    }
    return 0;
}

static int b3d_perception_ini_apply_compact(const char *key,
                                            const char *value,
                                            Blank3DTruthGate *truth_gate,
                                            g3d_fix *truth_range,
                                            Blank3DPerceptionConfig *config)
{
    if (b3d_perception_ini_equal(key, "range") ||
        b3d_perception_ini_equal(key, "view_range") ||
        b3d_perception_ini_equal(key, "viewrange") ||
        b3d_perception_ini_equal(key, "perception_range") ||
        b3d_perception_ini_equal(key, "perceptionrange")) {
        if (config)
            config->view_range = b3d_perception_ini_fixed(
                value, config->view_range);
        if (truth_range && config) *truth_range = config->view_range;
        return 1;
    }
    if (b3d_perception_ini_apply_truth(key, value,
                                       truth_gate, truth_range)) return 1;
    if (b3d_perception_ini_apply_eyes(key, value, config)) return 1;
    if (b3d_perception_ini_apply_hearing(key, value, config)) return 1;
    return 0;
}

int blank3d_perception_ini_load(const char *path,
                                Blank3DTruthGate *truth_gate,
                                g3d_fix *truth_range,
                                Blank3DPerceptionConfig *perception,
                                char *status,
                                size_t status_capacity)
{
    FILE *file;
    char line[B3D_PERCEPTION_INI_LINE_CAP];
    char section[48];
    int recognized;
    if (!path || !perception) {
        b3d_perception_ini_status(status, status_capacity,
                                  "invalid perception INI arguments");
        return 0;
    }
    file = fopen(path, "rb");
    if (!file) {
        b3d_perception_ini_status(status, status_capacity,
                                  "perception INI missing");
        return 0;
    }
    section[0] = '\0';
    recognized = 0;
    while (fgets(line, sizeof(line), file)) {
        char *p;
        char *close;
        char *eq;
        char *key;
        char *value;
        char *comment;
        p = b3d_perception_ini_trim(line);
        if (!*p || *p == ';' || *p == '#') continue;
        if (*p == '[') {
            close = strchr(p, ']');
            if (close) {
                *close = '\0';
                strncpy(section, b3d_perception_ini_trim(p + 1),
                        sizeof(section) - 1U);
                section[sizeof(section) - 1U] = '\0';
            }
            continue;
        }
        eq = strchr(p, '=');
        if (!eq) continue;
        *eq = '\0';
        key = b3d_perception_ini_trim(p);
        value = b3d_perception_ini_trim(eq + 1);
        comment = strchr(value, ';');
        if (comment) *comment = '\0';
        comment = strchr(value, '#');
        if (comment) *comment = '\0';
        value = b3d_perception_ini_trim(value);

        if (b3d_perception_ini_is_section(section, "perception") ||
            b3d_perception_ini_is_section(section, "senses") ||
            b3d_perception_ini_is_section(section, "sensory_truth")) {
            if (b3d_perception_ini_apply_compact(key, value,
                    truth_gate, truth_range, perception)) ++recognized;
        } else if (b3d_perception_ini_is_section(section, "truth") ||
                   b3d_perception_ini_is_section(section, "geder")) {
            if (b3d_perception_ini_apply_truth(key, value,
                    truth_gate, truth_range)) ++recognized;
        } else if (b3d_perception_ini_is_section(section, "eyes") ||
                   b3d_perception_ini_is_section(section, "vision")) {
            if (b3d_perception_ini_apply_eyes(key, value, perception))
                ++recognized;
        } else if (b3d_perception_ini_is_section(section, "hearing") ||
                   b3d_perception_ini_is_section(section, "ears")) {
            if (b3d_perception_ini_apply_hearing(key, value, perception))
                ++recognized;
        }
    }
    fclose(file);
    if (recognized == 0) {
        b3d_perception_ini_status(status, status_capacity,
                                  "INI loaded; no perception section");
        return 1;
    }
    b3d_perception_ini_status(status, status_capacity,
                              "perception INI loaded");
    return 1;
}
