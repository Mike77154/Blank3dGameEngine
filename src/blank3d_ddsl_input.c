#include "blank3d_ddsl_input.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

static void b3d_copy(char *dst, unsigned int cap, const char *src)
{
    unsigned int i;
    if (!dst || cap == 0U) return;
    if (!src) src = "";
    i = 0U;
    while (src[i] != '\0' && i + 1U < cap) {
        dst[i] = src[i];
        ++i;
    }
    dst[i] = '\0';
}

static int b3d_ci_equal(const char *a, const char *b)
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

static void b3d_normalize_control(const char *source, char *out,
                                  unsigned int capacity)
{
    unsigned int i;
    unsigned int j;
    unsigned char c;
    int last_sep;
    if (!out || capacity == 0U) return;
    if (!source) source = "";
    j = 0U;
    last_sep = 0;
    for (i = 0U; source[i] != '\0' && j + 1U < capacity; ++i) {
        c = (unsigned char)source[i];
        if (isalnum(c)) {
            out[j++] = (char)tolower(c);
            last_sep = 0;
        } else if (!last_sep && j > 0U) {
            out[j++] = '_';
            last_sep = 1;
        }
    }
    if (j > 0U && out[j - 1U] == '_') --j;
    out[j] = '\0';
}

static const char *b3d_state_alias(const char *token)
{
    if (b3d_ci_equal(token, "key_hold") ||
        b3d_ci_equal(token, "key_down")) return "hold";
    if (b3d_ci_equal(token, "key_pressed") ||
        b3d_ci_equal(token, "key_press")) return "pressed";
    if (b3d_ci_equal(token, "key_released") ||
        b3d_ci_equal(token, "key_release")) return "released";
    if (b3d_ci_equal(token, "key_repeat")) return "repeat";
    if (b3d_ci_equal(token, "key_tapped") ||
        b3d_ci_equal(token, "key_tap")) return "tapped";
    if (b3d_ci_equal(token, "key_long_hold") ||
        b3d_ci_equal(token, "key_longhold")) return "long_hold";
    return 0;
}

static int b3d_is_ident_char(unsigned char c)
{
    return isalnum(c) || c == '_' || c == '-';
}

static int b3d_append_char(char *out, unsigned int cap,
                           unsigned int *length, char c)
{
    if (!out || !length || *length + 1U >= cap) return 0;
    out[*length] = c;
    ++*length;
    out[*length] = '\0';
    return 1;
}

static int b3d_append_text(char *out, unsigned int cap,
                           unsigned int *length, const char *text)
{
    unsigned int i;
    if (!text) return 1;
    for (i = 0U; text[i] != '\0'; ++i)
        if (!b3d_append_char(out, cap, length, text[i])) return 0;
    return 1;
}

static int b3d_registry_find(const Blank3DDdslInputRegistry *registry,
                             const char *state, const char *control)
{
    int i;
    if (!registry) return -1;
    for (i = 0; i < registry->count; ++i) {
        if (strcmp(registry->refs[i].state, state) == 0 &&
            strcmp(registry->refs[i].control, control) == 0)
            return i;
    }
    return -1;
}

static int b3d_registry_add(Blank3DDdslInputRegistry *registry,
                            const char *state, const char *control)
{
    int index;
    if (!registry || !state || !control) return -1;
    index = b3d_registry_find(registry, state, control);
    if (index >= 0) return index;
    if (registry->count >= B3D_DDSL_INPUT_MAX_REFS) return -1;
    index = registry->count++;
    sprintf(registry->refs[index].store_name, "__input_%03d", index);
    b3d_copy(registry->refs[index].state,
             B3D_DDSL_INPUT_STATE_CAP, state);
    b3d_copy(registry->refs[index].control,
             B3D_DDSL_INPUT_CONTROL_CAP, control);
    return index;
}

void blank3d_ddsl_input_registry_init(Blank3DDdslInputRegistry *registry)
{
    if (!registry) return;
    memset(registry, 0, sizeof(*registry));
}

static void b3d_set_error(char *error, unsigned int cap, const char *text)
{
    b3d_copy(error, cap, text);
}

int blank3d_ddsl_input_preprocess(const char *source,
                                  char *output,
                                  unsigned int output_capacity,
                                  Blank3DDdslInputRegistry *registry,
                                  char *error,
                                  unsigned int error_capacity)
{
    unsigned int i;
    unsigned int out_len;
    unsigned int start;
    unsigned int end;
    unsigned int next;
    unsigned int control_start;
    unsigned int control_end;
    char token[80];
    char control[80];
    char normalized[80];
    const char *state;
    int ref_index;
    int in_string;
    int in_comment;
    int expect_then_action;
    if (!source || !output || output_capacity < 2U || !registry) return 0;
    blank3d_ddsl_input_registry_init(registry);
    if (error && error_capacity > 0U) error[0] = '\0';
    output[0] = '\0';
    out_len = 0U;
    i = 0U;
    in_string = 0;
    in_comment = 0;
    expect_then_action = 0;
    while (source[i] != '\0') {
        if (source[i] == '\n') in_comment = 0;
        if (!in_string && !in_comment && source[i] == '#') in_comment = 1;
        if (!in_comment && source[i] == '"') in_string = !in_string;
        if (!in_string && !in_comment &&
            b3d_is_ident_char((unsigned char)source[i])) {
            start = i;
            while (b3d_is_ident_char((unsigned char)source[i])) ++i;
            end = i;
            if (end - start >= sizeof(token)) {
                b3d_set_error(error, error_capacity, "DDSL input token too long");
                return 0;
            }
            memcpy(token, source + start, end - start);
            token[end - start] = '\0';
            state = b3d_state_alias(token);
            if (state) {
                next = end;
                while (source[next] == ' ' || source[next] == '\t') ++next;
                control_start = next;
                while (b3d_is_ident_char((unsigned char)source[next])) ++next;
                control_end = next;
                if (control_end == control_start) {
                    b3d_set_error(error, error_capacity,
                                  "key_* requires a control name");
                    return 0;
                }
                if (control_end - control_start >= sizeof(control)) {
                    b3d_set_error(error, error_capacity,
                                  "DDSL control name too long");
                    return 0;
                }
                memcpy(control, source + control_start,
                       control_end - control_start);
                control[control_end - control_start] = '\0';
                b3d_normalize_control(control, normalized,
                                      (unsigned int)sizeof(normalized));
                ref_index = b3d_registry_add(registry, state, normalized);
                if (ref_index < 0) {
                    b3d_set_error(error, error_capacity,
                                  "too many DDSL input references");
                    return 0;
                }
                if (!b3d_append_text(output, output_capacity, &out_len,
                                     registry->refs[ref_index].store_name)) {
                    b3d_set_error(error, error_capacity,
                                  "preprocessed DDSL source too large");
                    return 0;
                }
                i = control_end;
                continue;
            }
            if ((end - start) > 4U &&
                (token[0] == 'k' || token[0] == 'K') &&
                (token[1] == 'e' || token[1] == 'E') &&
                (token[2] == 'y' || token[2] == 'Y') &&
                token[3] == '-') {
                b3d_normalize_control(token + 4, normalized,
                                      (unsigned int)sizeof(normalized));
                ref_index = b3d_registry_add(registry, "hold", normalized);
                if (ref_index < 0) {
                    b3d_set_error(error, error_capacity,
                                  "too many DDSL input references");
                    return 0;
                }
                if (!b3d_append_text(output, output_capacity, &out_len,
                                     registry->refs[ref_index].store_name)) {
                    b3d_set_error(error, error_capacity,
                                  "preprocessed DDSL source too large");
                    return 0;
                }
                continue;
            }
            if (expect_then_action) {
                unsigned int look;
                look = end;
                while (source[look] == ' ' || source[look] == '\t' ||
                       source[look] == '\r') ++look;
                while (start < end) {
                    if (!b3d_append_char(output, output_capacity, &out_len,
                                         source[start++])) {
                        b3d_set_error(error, error_capacity,
                                      "preprocessed DDSL source too large");
                        return 0;
                    }
                }
                if (source[look] == '\0' || source[look] == '\n' ||
                    source[look] == '#') {
                    if (!b3d_append_text(output, output_capacity, &out_len,
                                         " = 1")) {
                        b3d_set_error(error, error_capacity,
                                      "preprocessed DDSL source too large");
                        return 0;
                    }
                }
                expect_then_action = 0;
                continue;
            }
            while (start < end) {
                if (!b3d_append_char(output, output_capacity, &out_len,
                                     source[start++])) {
                    b3d_set_error(error, error_capacity,
                                  "preprocessed DDSL source too large");
                    return 0;
                }
            }
            if (b3d_ci_equal(token, "then")) expect_then_action = 1;
            continue;
        }
        if (!b3d_append_char(output, output_capacity, &out_len, source[i])) {
            b3d_set_error(error, error_capacity,
                          "preprocessed DDSL source too large");
            return 0;
        }
        ++i;
    }
    return 1;
}
