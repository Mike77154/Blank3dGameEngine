#include "blank3d_weapon_presentation.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define B3D_WPRES_LINE_CAP 512

static void b3d_wpres_copy(char *dst, size_t cap, const char *src)
{
    size_t i;
    if (dst == 0 || cap == 0U) return;
    if (src == 0) src = "";
    i = 0U;
    while (i + 1U < cap && src[i] != '\0') {
        dst[i] = src[i];
        ++i;
    }
    dst[i] = '\0';
}

static char *b3d_wpres_trim(char *text)
{
    char *end;
    if (text == 0) return text;
    while (*text && isspace((unsigned char)*text)) ++text;
    end = text + strlen(text);
    while (end > text && isspace((unsigned char)end[-1])) --end;
    *end = '\0';
    return text;
}

static int b3d_wpres_eq(const char *a, const char *b)
{
    unsigned char ca;
    unsigned char cb;
    if (a == 0 || b == 0) return 0;
    while (*a && *b) {
        ca = (unsigned char)tolower((unsigned char)*a);
        cb = (unsigned char)tolower((unsigned char)*b);
        if (ca != cb) return 0;
        ++a;
        ++b;
    }
    return *a == '\0' && *b == '\0';
}

static int b3d_wpres_int(const char *text, int fallback)
{
    char *end;
    long value;
    if (text == 0 || *text == '\0') return fallback;
    value = strtol(text, &end, 10);
    end = b3d_wpres_trim(end);
    if (*end != '\0') return fallback;
    if (value < -32768L) value = -32768L;
    if (value > 32767L) value = 32767L;
    return (int)value;
}

static int b3d_wpres_bool(const char *text, int fallback)
{
    if (text == 0) return fallback;
    if (b3d_wpres_eq(text, "true") || b3d_wpres_eq(text, "yes") ||
        b3d_wpres_eq(text, "on")) return 1;
    if (b3d_wpres_eq(text, "false") || b3d_wpres_eq(text, "no") ||
        b3d_wpres_eq(text, "off")) return 0;
    return b3d_wpres_int(text, fallback) != 0;
}

static gatt_fix b3d_wpres_fix(const char *text, gatt_fix fallback)
{
    long q12;
    if (text == 0 || *text == '\0') return fallback;
    /* GWeapon89 and GAttach89 both use Q20.12.  Keep the value in that
       domain here; NationalMecanicanimal89 performs the single Q12->Q16
       conversion at its provider boundary. */
    q12 = gwp89_fx_from_text(text);
    if (q12 > 2147483647L) q12 = 2147483647L;
    if (q12 < (-2147483647L - 1L)) q12 = (-2147483647L - 1L);
    return (gatt_fix)q12;
}

static void b3d_wpres_manifest_dir(const char *path,
                                   char *directory,
                                   size_t capacity)
{
    const char *slash;
    const char *backslash;
    size_t length;
    if (directory == 0 || capacity == 0U) return;
    directory[0] = '\0';
    if (path == 0) return;
    slash = strrchr(path, '/');
    backslash = strrchr(path, '\\');
    if (backslash != 0 && (slash == 0 || backslash > slash)) slash = backslash;
    if (slash == 0) return;
    length = (size_t)(slash - path + 1);
    if (length >= capacity) length = capacity - 1U;
    memcpy(directory, path, length);
    directory[length] = '\0';
}

static void b3d_wpres_join(const char *directory,
                           const char *leaf,
                           char *out,
                           size_t capacity)
{
    if (out == 0 || capacity == 0U) return;
    out[0] = '\0';
    b3d_wpres_copy(out, capacity, directory);
    if (strlen(out) + strlen(leaf ? leaf : "") + 1U < capacity)
        strcat(out, leaf ? leaf : "");
}

int blank3d_weapon_presentation_mechanism_from_text(const char *text)
{
    if (b3d_wpres_eq(text, "slide_magazine") ||
        b3d_wpres_eq(text, "slide") || b3d_wpres_eq(text, "automatic"))
        return B3D_MECHANISM_SLIDE_MAGAZINE;
    if (b3d_wpres_eq(text, "bolt_magazine") || b3d_wpres_eq(text, "bolt"))
        return B3D_MECHANISM_BOLT_MAGAZINE;
    if (b3d_wpres_eq(text, "pump_tube") || b3d_wpres_eq(text, "pump"))
        return B3D_MECHANISM_PUMP_TUBE;
    if (b3d_wpres_eq(text, "revolver") || b3d_wpres_eq(text, "cylinder"))
        return B3D_MECHANISM_REVOLVER;
    if (b3d_wpres_eq(text, "gatling_belt") || b3d_wpres_eq(text, "gatling"))
        return B3D_MECHANISM_GATLING_BELT;
    if (b3d_wpres_eq(text, "elastic") || b3d_wpres_eq(text, "slingshot"))
        return B3D_MECHANISM_ELASTIC;
    return B3D_MECHANISM_FIXED;
}

const char *blank3d_weapon_presentation_mechanism_name(int kind)
{
    switch (kind) {
    case B3D_MECHANISM_SLIDE_MAGAZINE: return "slide_magazine";
    case B3D_MECHANISM_BOLT_MAGAZINE: return "bolt_magazine";
    case B3D_MECHANISM_PUMP_TUBE: return "pump_tube";
    case B3D_MECHANISM_REVOLVER: return "revolver";
    case B3D_MECHANISM_GATLING_BELT: return "gatling_belt";
    case B3D_MECHANISM_ELASTIC: return "elastic";
    default: return "fixed";
    }
}

void blank3d_weapon_presentation_defaults(
    Blank3DWeaponPresentation *profile,
    int weapon_id,
    const char *weapon_name)
{
    if (profile == 0) return;
    memset(profile, 0, sizeof(*profile));
    profile->used = 1;
    profile->weapon_id = weapon_id;
    profile->model_id = weapon_id > 0 ? weapon_id : B3D_WEAPON_MODEL_GENERIC;
    profile->mechanism_kind = B3D_MECHANISM_SLIDE_MAGAZINE;
    profile->detachable_magazine = 1;
    profile->fire_ticks = 7U;
    b3d_wpres_copy(profile->weapon_name, sizeof(profile->weapon_name),
                    weapon_name ? weapon_name : "weapon");
    b3d_wpres_copy(profile->model_name, sizeof(profile->model_name),
                    weapon_name ? weapon_name : "generic_weapon");
    b3d_wpres_copy(profile->socket_name, sizeof(profile->socket_name),
                    "weapon_r");
    b3d_wpres_copy(profile->attachment_name,
                    sizeof(profile->attachment_name), "equipped_weapon");
    profile->grip_offset = gatt89_xform_identity();
    profile->recoil_z = gatt89_div(gatt89_from_int(3), gatt89_from_int(40));
    profile->recoil_pitch = gatt89_from_int(-2);
    profile->action_home_z = gatt89_div(gatt89_from_int(-2), gatt89_from_int(25));
    profile->action_fire_z = gatt89_div(gatt89_from_int(7), gatt89_from_int(50));
    profile->feed_home_y = gatt89_div(gatt89_from_int(-5), gatt89_from_int(16));
    profile->feed_home_z = gatt89_div(gatt89_from_int(3), gatt89_from_int(20));
    profile->feed_out_y = gatt89_div(gatt89_from_int(-17), gatt89_from_int(20));
    profile->feed_out_z = gatt89_div(gatt89_from_int(1), gatt89_from_int(4));
    profile->barrel_home_z = gatt89_div(gatt89_from_int(-31), gatt89_from_int(50));
    profile->muzzle_y = gatt89_div(gatt89_from_int(1), gatt89_from_int(50));
    profile->muzzle_z = gatt89_div(gatt89_from_int(-43), gatt89_from_int(50));
}

void blank3d_weapon_presentation_registry_init(
    Blank3DWeaponPresentationRegistry *registry)
{
    if (registry == 0) return;
    memset(registry, 0, sizeof(*registry));
    b3d_wpres_copy(registry->status, sizeof(registry->status),
                    "weapon presentation registry initialized");
}

static void b3d_wpres_apply(Blank3DWeaponPresentation *profile,
                            const char *key,
                            const char *value)
{
    if (b3d_wpres_eq(key, "model_id"))
        profile->model_id = b3d_wpres_int(value, profile->model_id);
    else if (b3d_wpres_eq(key, "model") || b3d_wpres_eq(key, "model_name"))
        b3d_wpres_copy(profile->model_name, sizeof(profile->model_name), value);
    else if (b3d_wpres_eq(key, "mechanism") || b3d_wpres_eq(key, "animator"))
        profile->mechanism_kind =
            blank3d_weapon_presentation_mechanism_from_text(value);
    else if (b3d_wpres_eq(key, "socket"))
        b3d_wpres_copy(profile->socket_name, sizeof(profile->socket_name), value);
    else if (b3d_wpres_eq(key, "attachment"))
        b3d_wpres_copy(profile->attachment_name,
                        sizeof(profile->attachment_name), value);
    else if (b3d_wpres_eq(key, "detachable_magazine"))
        profile->detachable_magazine =
            b3d_wpres_bool(value, profile->detachable_magazine);
    else if (b3d_wpres_eq(key, "fire_ticks"))
        profile->fire_ticks = (unsigned short)b3d_wpres_int(value,
                                                (int)profile->fire_ticks);
    else if (b3d_wpres_eq(key, "grip_x"))
        profile->grip_offset.pos.x = b3d_wpres_fix(value,
                                                   profile->grip_offset.pos.x);
    else if (b3d_wpres_eq(key, "grip_y"))
        profile->grip_offset.pos.y = b3d_wpres_fix(value,
                                                   profile->grip_offset.pos.y);
    else if (b3d_wpres_eq(key, "grip_z"))
        profile->grip_offset.pos.z = b3d_wpres_fix(value,
                                                   profile->grip_offset.pos.z);
    else if (b3d_wpres_eq(key, "scale_x"))
        profile->grip_offset.scale.x = b3d_wpres_fix(value,
                                                     profile->grip_offset.scale.x);
    else if (b3d_wpres_eq(key, "scale_y"))
        profile->grip_offset.scale.y = b3d_wpres_fix(value,
                                                     profile->grip_offset.scale.y);
    else if (b3d_wpres_eq(key, "scale_z"))
        profile->grip_offset.scale.z = b3d_wpres_fix(value,
                                                     profile->grip_offset.scale.z);
    else if (b3d_wpres_eq(key, "recoil_z"))
        profile->recoil_z = b3d_wpres_fix(value, profile->recoil_z);
    else if (b3d_wpres_eq(key, "recoil_pitch"))
        profile->recoil_pitch = b3d_wpres_fix(value, profile->recoil_pitch);
    else if (b3d_wpres_eq(key, "action_home_z"))
        profile->action_home_z = b3d_wpres_fix(value, profile->action_home_z);
    else if (b3d_wpres_eq(key, "action_fire_z"))
        profile->action_fire_z = b3d_wpres_fix(value, profile->action_fire_z);
    else if (b3d_wpres_eq(key, "feed_home_y"))
        profile->feed_home_y = b3d_wpres_fix(value, profile->feed_home_y);
    else if (b3d_wpres_eq(key, "feed_home_z"))
        profile->feed_home_z = b3d_wpres_fix(value, profile->feed_home_z);
    else if (b3d_wpres_eq(key, "feed_out_y"))
        profile->feed_out_y = b3d_wpres_fix(value, profile->feed_out_y);
    else if (b3d_wpres_eq(key, "feed_out_z"))
        profile->feed_out_z = b3d_wpres_fix(value, profile->feed_out_z);
    else if (b3d_wpres_eq(key, "barrel_home_z"))
        profile->barrel_home_z = b3d_wpres_fix(value, profile->barrel_home_z);
    else if (b3d_wpres_eq(key, "muzzle_y"))
        profile->muzzle_y = b3d_wpres_fix(value, profile->muzzle_y);
    else if (b3d_wpres_eq(key, "muzzle_z"))
        profile->muzzle_z = b3d_wpres_fix(value, profile->muzzle_z);
}

static int b3d_wpres_load_file(const char *path,
                                Blank3DWeaponPresentation *profile)
{
    FILE *file;
    char line[B3D_WPRES_LINE_CAP];
    char section[48];
    int weapon_id;
    char weapon_name[B3D_WPRES_NAME_CAP];
    int found_presentation;
    if (path == 0 || profile == 0) return 0;
    file = fopen(path, "rb");
    if (file == 0) return 0;
    section[0] = '\0';
    weapon_id = 0;
    weapon_name[0] = '\0';
    found_presentation = 0;
    while (fgets(line, sizeof(line), file)) {
        char *text;
        char *close;
        char *equals;
        char *key;
        char *value;
        text = b3d_wpres_trim(line);
        if (*text == '\0' || *text == ';' || *text == '#') continue;
        if (*text == '[') {
            close = strchr(text + 1, ']');
            if (close == 0) continue;
            *close = '\0';
            b3d_wpres_copy(section, sizeof(section),
                            b3d_wpres_trim(text + 1));
            continue;
        }
        equals = strchr(text, '=');
        if (equals == 0) continue;
        *equals = '\0';
        key = b3d_wpres_trim(text);
        value = b3d_wpres_trim(equals + 1);
        if (b3d_wpres_eq(section, "weapon")) {
            if (b3d_wpres_eq(key, "id"))
                weapon_id = b3d_wpres_int(value, weapon_id);
            else if (b3d_wpres_eq(key, "name"))
                b3d_wpres_copy(weapon_name, sizeof(weapon_name), value);
        }
    }
    rewind(file);
    blank3d_weapon_presentation_defaults(profile, weapon_id, weapon_name);
    section[0] = '\0';
    while (fgets(line, sizeof(line), file)) {
        char *text;
        char *close;
        char *equals;
        char *key;
        char *value;
        text = b3d_wpres_trim(line);
        if (*text == '\0' || *text == ';' || *text == '#') continue;
        if (*text == '[') {
            close = strchr(text + 1, ']');
            if (close == 0) continue;
            *close = '\0';
            b3d_wpres_copy(section, sizeof(section),
                            b3d_wpres_trim(text + 1));
            continue;
        }
        if (!b3d_wpres_eq(section, "presentation") &&
            !b3d_wpres_eq(section, "model") &&
            !b3d_wpres_eq(section, "mechanism")) continue;
        equals = strchr(text, '=');
        if (equals == 0) continue;
        *equals = '\0';
        key = b3d_wpres_trim(text);
        value = b3d_wpres_trim(equals + 1);
        b3d_wpres_apply(profile, key, value);
        found_presentation = 1;
    }
    fclose(file);
    return profile->weapon_id > 0 && (found_presentation || profile->used);
}

int blank3d_weapon_presentation_load_manifest(
    Blank3DWeaponPresentationRegistry *registry,
    const char *manifest_path,
    char *status,
    size_t status_capacity)
{
    FILE *manifest;
    char line[B3D_WPRES_LINE_CAP];
    char section[48];
    char directory[B3D_WPRES_PATH_CAP];
    char path[B3D_WPRES_PATH_CAP];
    int count;
    if (registry == 0 || manifest_path == 0) return 0;
    blank3d_weapon_presentation_registry_init(registry);
    manifest = fopen(manifest_path, "rb");
    if (manifest == 0) {
        b3d_wpres_copy(registry->status, sizeof(registry->status),
                        "weapon presentation manifest missing");
        b3d_wpres_copy(status, status_capacity, registry->status);
        return 0;
    }
    b3d_wpres_manifest_dir(manifest_path, directory, sizeof(directory));
    section[0] = '\0';
    count = 0;
    while (fgets(line, sizeof(line), manifest)) {
        char *text;
        char *close;
        char *equals;
        char *value;
        text = b3d_wpres_trim(line);
        if (*text == '\0' || *text == ';' || *text == '#') continue;
        if (*text == '[') {
            close = strchr(text + 1, ']');
            if (close == 0) continue;
            *close = '\0';
            b3d_wpres_copy(section, sizeof(section),
                            b3d_wpres_trim(text + 1));
            continue;
        }
        if (!b3d_wpres_eq(section, "weapons")) continue;
        equals = strchr(text, '=');
        if (equals == 0) continue;
        value = b3d_wpres_trim(equals + 1);
        if (*value == '\0') continue;
        if (count >= B3D_WPRES_MAX_PROFILES) break;
        b3d_wpres_join(directory, value, path, sizeof(path));
        if (b3d_wpres_load_file(path, &registry->profiles[count])) ++count;
    }
    fclose(manifest);
    registry->count = count;
    if (count > 0) {
        char message[160];
        sprintf(message, "weapon presentation registry loaded: %d profiles", count);
        b3d_wpres_copy(registry->status, sizeof(registry->status), message);
    } else {
        b3d_wpres_copy(registry->status, sizeof(registry->status),
                        "weapon presentation registry contained no profiles");
    }
    b3d_wpres_copy(status, status_capacity, registry->status);
    return count;
}

const Blank3DWeaponPresentation *blank3d_weapon_presentation_find(
    const Blank3DWeaponPresentationRegistry *registry,
    int weapon_id)
{
    int i;
    if (registry == 0) return 0;
    for (i = 0; i < B3D_WPRES_MAX_PROFILES; ++i) {
        if (registry->profiles[i].used &&
            registry->profiles[i].weapon_id == weapon_id)
            return &registry->profiles[i];
    }
    return 0;
}
