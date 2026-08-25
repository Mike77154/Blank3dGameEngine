#include "blank3d_camera_profiles.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#endif

static void b3d_camera_copy_text(char *dst,
                                 unsigned int capacity,
                                 const char *src)
{
    unsigned int i;
    if (!dst || capacity == 0U) return;
    if (!src) src = "";
    i = 0U;
    while (src[i] != '\0' && i + 1U < capacity) {
        dst[i] = src[i];
        ++i;
    }
    dst[i] = '\0';
}

static void b3d_camera_copy_slice(char *dst,
                                  unsigned int capacity,
                                  conf_slice_t value,
                                  const char *fallback)
{
    unsigned int i;
    if (!dst || capacity == 0U) return;
    if (!value.ptr) {
        b3d_camera_copy_text(dst, capacity, fallback);
        return;
    }
    i = 0U;
    while (i < value.len && i + 1U < capacity) {
        dst[i] = value.ptr[i];
        ++i;
    }
    dst[i] = '\0';
}

static int b3d_camera_text_equal(const char *a, const char *b)
{
    unsigned char ca;
    unsigned char cb;
    if (!a || !b) return 0;
    while (*a != '\0' && *b != '\0') {
        ca = (unsigned char)*a;
        cb = (unsigned char)*b;
        if (tolower(ca) != tolower(cb)) return 0;
        ++a;
        ++b;
    }
    return *a == '\0' && *b == '\0';
}

static int b3d_camera_has_ini_extension(const char *name)
{
    unsigned int length;
    if (!name) return 0;
    length = (unsigned int)strlen(name);
    if (length < 4U) return 0;
    return b3d_camera_text_equal(name + length - 4U, ".ini");
}

static int b3d_camera_join_path(char *out,
                                unsigned int capacity,
                                const char *directory,
                                const char *name)
{
    unsigned int dlen;
    unsigned int nlen;
    char separator;
    if (!out || !directory || !name || capacity == 0U) return 0;
    dlen = (unsigned int)strlen(directory);
    nlen = (unsigned int)strlen(name);
#ifdef _WIN32
    separator = '\\';
#else
    separator = '/';
#endif
    if (dlen + nlen + 2U > capacity) return 0;
    memcpy(out, directory, dlen);
    if (dlen > 0U && directory[dlen - 1U] != '/' &&
        directory[dlen - 1U] != '\\') {
        out[dlen++] = separator;
    }
    memcpy(out + dlen, name, nlen);
    out[dlen + nlen] = '\0';
    return 1;
}

static void b3d_camera_basename_id(char *out,
                                   unsigned int capacity,
                                   const char *path)
{
    const char *base;
    const char *cursor;
    unsigned int length;
    if (!out || capacity == 0U) return;
    base = path ? path : "camera";
    cursor = base;
    while (*cursor != '\0') {
        if (*cursor == '/' || *cursor == '\\') base = cursor + 1;
        ++cursor;
    }
    length = (unsigned int)strlen(base);
    if (length > 4U && b3d_camera_has_ini_extension(base)) length -= 4U;
    if (length + 1U > capacity) length = capacity - 1U;
    memcpy(out, base, length);
    out[length] = '\0';
}

static g3d_fix b3d_camera_q16_to_q20(conf_fixed_t value)
{
    return (g3d_fix)(value / 16L);
}

static conf_slice_t b3d_camera_empty_slice(void)
{
    conf_slice_t result;
    result.ptr = (const char *)0;
    result.len = 0U;
    return result;
}

static int b3d_camera_view_style_from_text(const char *text, int fallback)
{
    if (b3d_camera_text_equal(text, "fps") ||
        b3d_camera_text_equal(text, "first_person"))
        return GWP89_VIEW_FPS;
    if (b3d_camera_text_equal(text, "ots") ||
        b3d_camera_text_equal(text, "over_shoulder") ||
        b3d_camera_text_equal(text, "over_the_shoulder"))
        return GWP89_VIEW_OVER_SHOULDER;
    if (b3d_camera_text_equal(text, "tps") ||
        b3d_camera_text_equal(text, "third_person"))
        return GWP89_VIEW_THIRD_PERSON;
    return fallback;
}

static int b3d_camera_rig_from_text(const char *text, int fallback)
{
    if (b3d_camera_text_equal(text, "fps") ||
        b3d_camera_text_equal(text, "first_person"))
        return B3D_CAMERA_RIG_FPS;
    if (b3d_camera_text_equal(text, "orbit") ||
        b3d_camera_text_equal(text, "third_person") ||
        b3d_camera_text_equal(text, "tps") ||
        b3d_camera_text_equal(text, "ots"))
        return B3D_CAMERA_RIG_ORBIT;
    return fallback;
}

static int b3d_camera_aim_mode_from_text(const char *text, int fallback)
{
    if (b3d_camera_text_equal(text, "camera") ||
        b3d_camera_text_equal(text, "camera_only"))
        return B3D_CAMERA_AIM_CAMERA_ONLY;
    if (b3d_camera_text_equal(text, "camera_muzzle") ||
        b3d_camera_text_equal(text, "two_ray") ||
        b3d_camera_text_equal(text, "camera_then_muzzle"))
        return B3D_CAMERA_AIM_CAMERA_MUZZLE;
    return fallback;
}

void blank3d_camera_profile_defaults(Blank3DCameraProfile *profile,
                                     int rig,
                                     int view_style)
{
    if (!profile) return;
    memset(profile, 0, sizeof(*profile));
    profile->loaded = 1;
    profile->enabled = 1;
    profile->order = 100;
    profile->rig = rig;
    profile->view_style = view_style;
    profile->aim_mode = rig == B3D_CAMERA_RIG_FPS
                      ? B3D_CAMERA_AIM_CAMERA_ONLY
                      : B3D_CAMERA_AIM_CAMERA_MUZZLE;
    profile->player_body_visible = rig == B3D_CAMERA_RIG_FPS ? 0 : 1;
    profile->player_weapon_visible = rig == B3D_CAMERA_RIG_FPS ? 0 : 1;
    profile->collision_enabled = rig == B3D_CAMERA_RIG_ORBIT ? 1 : 0;
    profile->pivot_y = rig == B3D_CAMERA_RIG_FPS
                     ? (G3D_FIX_ONE * 17L / 10L)
                     : G3D_FIX_FROM_INT(4);
    profile->distance = rig == B3D_CAMERA_RIG_FPS
                      ? 0 : G3D_FIX_FROM_INT(8);
    profile->min_distance = G3D_FIX_ONE / 2L;
    profile->max_distance = G3D_FIX_FROM_INT(64);
    profile->pitch_min = G3D_FIX_FROM_INT(-85);
    profile->pitch_max = G3D_FIX_FROM_INT(85);
    profile->fov = G3D_FIX_FROM_INT(70);
    profile->near_clip = G3D_FIX_ONE / 10L;
    profile->far_clip = G3D_FIX_FROM_INT(200);
    profile->pos_lag = rig == B3D_CAMERA_RIG_FPS
                     ? G3D_FIX_ONE : (G3D_FIX_ONE * 45L / 100L);
    profile->rot_lag = rig == B3D_CAMERA_RIG_FPS
                     ? G3D_FIX_ONE : (G3D_FIX_ONE * 55L / 100L);
    profile->fov_lag = rig == B3D_CAMERA_RIG_FPS
                     ? G3D_FIX_ONE : (G3D_FIX_ONE * 55L / 100L);
    profile->mouse_sensitivity = G3D_FIX_ONE * 18L / 100L;
    profile->collision_radius = G3D_FIX_ONE * 3L / 10L;
    b3d_camera_copy_text(profile->id,
                         B3D_CAMERA_PROFILE_ID_CAPACITY,
                         rig == B3D_CAMERA_RIG_FPS ? "fps" : "tps_centred");
    b3d_camera_copy_text(profile->name,
                         B3D_CAMERA_PROFILE_NAME_CAPACITY,
                         rig == B3D_CAMERA_RIG_FPS ? "FPS" : "TPS Centred");
}

static void b3d_camera_add_fallback(Blank3DCameraCatalog *catalog,
                                    const char *id,
                                    const char *name,
                                    int order,
                                    int rig,
                                    int view_style,
                                    g3d_fix right_offset)
{
    Blank3DCameraProfile *profile;
    if (!catalog || catalog->count >= B3D_CAMERA_PROFILE_MAX) return;
    profile = &catalog->profiles[catalog->count++];
    blank3d_camera_profile_defaults(profile, rig, view_style);
    b3d_camera_copy_text(profile->id,
                         B3D_CAMERA_PROFILE_ID_CAPACITY, id);
    b3d_camera_copy_text(profile->name,
                         B3D_CAMERA_PROFILE_NAME_CAPACITY, name);
    b3d_camera_copy_text(profile->source_path,
                         B3D_CAMERA_PROFILE_PATH_CAPACITY,
                         "built-in fallback");
    profile->order = order;
    profile->offset_right = right_offset;
}

void blank3d_camera_catalog_init(Blank3DCameraCatalog *catalog)
{
    if (!catalog) return;
    memset(catalog, 0, sizeof(*catalog));
    b3d_camera_add_fallback(catalog, "tps_centred", "TPS Centred", 10,
                            B3D_CAMERA_RIG_ORBIT,
                            GWP89_VIEW_THIRD_PERSON, 0);
    b3d_camera_add_fallback(catalog, "ots", "Over the Shoulder", 20,
                            B3D_CAMERA_RIG_ORBIT,
                            GWP89_VIEW_OVER_SHOULDER,
                            G3D_FIX_FROM_INT(1));
    b3d_camera_add_fallback(catalog, "fps", "First Person", 30,
                            B3D_CAMERA_RIG_FPS,
                            GWP89_VIEW_FPS, 0);
    catalog->active_index = 0;
    b3d_camera_copy_text(catalog->status, sizeof(catalog->status),
                         "built-in camera fallbacks active");
}

static int b3d_camera_read_file(Blank3DCameraCatalog *catalog,
                                const char *path,
                                unsigned int *out_length)
{
    FILE *file;
    long size;
    unsigned int count;
    if (!catalog || !path || !out_length) return 0;
    file = fopen(path, "rb");
    if (!file) return 0;
    if (fseek(file, 0L, SEEK_END) != 0) {
        fclose(file);
        return 0;
    }
    size = ftell(file);
    if (size < 0L || size >= B3D_CAMERA_PROFILE_TEXT_CAPACITY) {
        fclose(file);
        return 0;
    }
    rewind(file);
    count = (unsigned int)fread(catalog->text, 1U, (size_t)size, file);
    fclose(file);
    catalog->text[count] = '\0';
    *out_length = count;
    return 1;
}

static int b3d_camera_load_profile_file(Blank3DCameraCatalog *catalog,
                                        const char *path,
                                        Blank3DCameraProfile *out_profile)
{
    conf_ctx_t parser;
    conf_err_t error;
    conf_slice_t empty;
    conf_slice_t value;
    unsigned int length;
    char id[B3D_CAMERA_PROFILE_ID_CAPACITY];
    char rig_text[32];
    char view_text[32];
    char aim_text[40];
    int rig;
    int view_style;

    if (!catalog || !path || !out_profile) return 0;
    if (!b3d_camera_read_file(catalog, path, &length)) return 0;
    conf_ctx_init(&parser, catalog->arena,
                  B3D_CAMERA_PROFILE_ARENA_CAPACITY);
    conf_ctx_set_load_flags(&parser,
                            CONF_LOAD_OVERRIDE | CONF_LOAD_COPY_SLICES);
    error = conf_load_ini(&parser, catalog->text, length);
    if (error != CONF_OK) return 0;

    empty = b3d_camera_empty_slice();
    b3d_camera_basename_id(id, sizeof(id), path);
    value = conf_get_string(&parser, "profile.rig", empty);
    b3d_camera_copy_slice(rig_text, sizeof(rig_text), value, "orbit");
    rig = b3d_camera_rig_from_text(rig_text, B3D_CAMERA_RIG_ORBIT);

    value = conf_get_string(&parser, "profile.view_style", empty);
    b3d_camera_copy_slice(view_text, sizeof(view_text), value,
                          rig == B3D_CAMERA_RIG_FPS ? "fps" : "third_person");
    view_style = b3d_camera_view_style_from_text(
        view_text,
        rig == B3D_CAMERA_RIG_FPS
            ? GWP89_VIEW_FPS : GWP89_VIEW_THIRD_PERSON);

    blank3d_camera_profile_defaults(out_profile, rig, view_style);
    b3d_camera_copy_text(out_profile->source_path,
                         B3D_CAMERA_PROFILE_PATH_CAPACITY, path);

    value = conf_get_string(&parser, "profile.id", empty);
    b3d_camera_copy_slice(out_profile->id,
                          B3D_CAMERA_PROFILE_ID_CAPACITY, value, id);
    value = conf_get_string(&parser, "profile.name", empty);
    b3d_camera_copy_slice(out_profile->name,
                          B3D_CAMERA_PROFILE_NAME_CAPACITY,
                          value, out_profile->id);
    out_profile->enabled = conf_get_bool(&parser, "profile.enabled", 1);
    out_profile->order = (int)conf_get_int(&parser, "profile.order", 100L);

    value = conf_get_string(&parser, "profile.aim", empty);
    b3d_camera_copy_slice(aim_text, sizeof(aim_text), value,
                          rig == B3D_CAMERA_RIG_FPS
                              ? "camera" : "camera_muzzle");
    out_profile->aim_mode = b3d_camera_aim_mode_from_text(
        aim_text, out_profile->aim_mode);
    out_profile->player_body_visible = conf_get_bool(
        &parser, "render.player_body", out_profile->player_body_visible);
    out_profile->player_weapon_visible = conf_get_bool(
        &parser, "render.player_weapon", out_profile->player_weapon_visible);

    out_profile->pivot_x = b3d_camera_q16_to_q20(conf_get_fixed(
        &parser, "rig.pivot_x", (conf_fixed_t)(out_profile->pivot_x * 16L)));
    out_profile->pivot_y = b3d_camera_q16_to_q20(conf_get_fixed(
        &parser, "rig.pivot_y", (conf_fixed_t)(out_profile->pivot_y * 16L)));
    out_profile->pivot_z = b3d_camera_q16_to_q20(conf_get_fixed(
        &parser, "rig.pivot_z", (conf_fixed_t)(out_profile->pivot_z * 16L)));
    out_profile->look_x = b3d_camera_q16_to_q20(conf_get_fixed(
        &parser, "rig.look_x", (conf_fixed_t)(out_profile->look_x * 16L)));
    out_profile->look_y = b3d_camera_q16_to_q20(conf_get_fixed(
        &parser, "rig.look_y", (conf_fixed_t)(out_profile->look_y * 16L)));
    out_profile->look_z = b3d_camera_q16_to_q20(conf_get_fixed(
        &parser, "rig.look_z", (conf_fixed_t)(out_profile->look_z * 16L)));
    out_profile->distance = b3d_camera_q16_to_q20(conf_get_fixed(
        &parser, "rig.distance", (conf_fixed_t)(out_profile->distance * 16L)));
    out_profile->min_distance = b3d_camera_q16_to_q20(conf_get_fixed(
        &parser, "rig.min_distance", (conf_fixed_t)(out_profile->min_distance * 16L)));
    out_profile->max_distance = b3d_camera_q16_to_q20(conf_get_fixed(
        &parser, "rig.max_distance", (conf_fixed_t)(out_profile->max_distance * 16L)));
    out_profile->offset_right = b3d_camera_q16_to_q20(conf_get_fixed(
        &parser, "rig.offset_right", (conf_fixed_t)(out_profile->offset_right * 16L)));
    out_profile->offset_up = b3d_camera_q16_to_q20(conf_get_fixed(
        &parser, "rig.offset_up", (conf_fixed_t)(out_profile->offset_up * 16L)));
    out_profile->offset_forward = b3d_camera_q16_to_q20(conf_get_fixed(
        &parser, "rig.offset_forward", (conf_fixed_t)(out_profile->offset_forward * 16L)));

    out_profile->pitch_min = b3d_camera_q16_to_q20(conf_get_fixed(
        &parser, "look.pitch_min", (conf_fixed_t)(out_profile->pitch_min * 16L)));
    out_profile->pitch_max = b3d_camera_q16_to_q20(conf_get_fixed(
        &parser, "look.pitch_max", (conf_fixed_t)(out_profile->pitch_max * 16L)));
    out_profile->mouse_sensitivity = b3d_camera_q16_to_q20(conf_get_fixed(
        &parser, "look.mouse_sensitivity",
        (conf_fixed_t)(out_profile->mouse_sensitivity * 16L)));

    out_profile->fov = b3d_camera_q16_to_q20(conf_get_fixed(
        &parser, "lens.fov", (conf_fixed_t)(out_profile->fov * 16L)));
    out_profile->near_clip = b3d_camera_q16_to_q20(conf_get_fixed(
        &parser, "lens.near", (conf_fixed_t)(out_profile->near_clip * 16L)));
    out_profile->far_clip = b3d_camera_q16_to_q20(conf_get_fixed(
        &parser, "lens.far", (conf_fixed_t)(out_profile->far_clip * 16L)));

    out_profile->pos_lag = b3d_camera_q16_to_q20(conf_get_fixed(
        &parser, "smoothing.position", (conf_fixed_t)(out_profile->pos_lag * 16L)));
    out_profile->rot_lag = b3d_camera_q16_to_q20(conf_get_fixed(
        &parser, "smoothing.rotation", (conf_fixed_t)(out_profile->rot_lag * 16L)));
    out_profile->fov_lag = b3d_camera_q16_to_q20(conf_get_fixed(
        &parser, "smoothing.fov", (conf_fixed_t)(out_profile->fov_lag * 16L)));

    out_profile->collision_enabled = conf_get_bool(
        &parser, "collision.enabled", out_profile->collision_enabled);
    out_profile->collision_radius = b3d_camera_q16_to_q20(conf_get_fixed(
        &parser, "collision.radius", (conf_fixed_t)(out_profile->collision_radius * 16L)));

    if (out_profile->pitch_min > out_profile->pitch_max) {
        g3d_fix swap;
        swap = out_profile->pitch_min;
        out_profile->pitch_min = out_profile->pitch_max;
        out_profile->pitch_max = swap;
    }
    if (out_profile->fov <= 0) out_profile->fov = G3D_FIX_FROM_INT(70);
    if (out_profile->near_clip <= 0) out_profile->near_clip = G3D_FIX_ONE / 10L;
    if (out_profile->far_clip <= out_profile->near_clip)
        out_profile->far_clip = G3D_FIX_FROM_INT(200);
    out_profile->loaded = 1;
    return out_profile->enabled;
}

static int b3d_camera_profile_before(const Blank3DCameraProfile *a,
                                     const Blank3DCameraProfile *b)
{
    if (a->order != b->order) return a->order < b->order;
    return strcmp(a->id, b->id) < 0;
}

static void b3d_camera_sort(Blank3DCameraCatalog *catalog)
{
    int i;
    int j;
    Blank3DCameraProfile swap;
    if (!catalog) return;
    for (i = 1; i < catalog->count; ++i) {
        j = i;
        while (j > 0 && b3d_camera_profile_before(
               &catalog->profiles[j], &catalog->profiles[j - 1])) {
            swap = catalog->profiles[j - 1];
            catalog->profiles[j - 1] = catalog->profiles[j];
            catalog->profiles[j] = swap;
            --j;
        }
    }
}

static int b3d_camera_catalog_add_path(Blank3DCameraCatalog *catalog,
                                       const char *path)
{
    Blank3DCameraProfile profile;
    if (!catalog || !path || catalog->count >= B3D_CAMERA_PROFILE_MAX)
        return 0;
    if (!b3d_camera_load_profile_file(catalog, path, &profile)) return 0;
    catalog->profiles[catalog->count++] = profile;
    return 1;
}

int blank3d_camera_catalog_load(Blank3DCameraCatalog *catalog,
                                const char *directory)
{
    int loaded;
    char path[B3D_CAMERA_PROFILE_PATH_CAPACITY];
#ifdef _WIN32
    WIN32_FIND_DATAA data;
    HANDLE search;
    char pattern[B3D_CAMERA_PROFILE_PATH_CAPACITY];
#else
    DIR *dir;
    struct dirent *entry;
#endif
    if (!catalog || !directory) return 0;
    catalog->count = 0;
    catalog->active_index = 0;
    catalog->loaded_from_directory = 0;
    b3d_camera_copy_text(catalog->directory, sizeof(catalog->directory),
                         directory);
    loaded = 0;
#ifdef _WIN32
    if (!b3d_camera_join_path(pattern, sizeof(pattern), directory, "*.ini"))
        return 0;
    search = FindFirstFileA(pattern, &data);
    if (search != INVALID_HANDLE_VALUE) {
        do {
            if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 &&
                b3d_camera_has_ini_extension(data.cFileName) &&
                b3d_camera_join_path(path, sizeof(path), directory,
                                     data.cFileName)) {
                loaded += b3d_camera_catalog_add_path(catalog, path);
            }
        } while (catalog->count < B3D_CAMERA_PROFILE_MAX &&
                 FindNextFileA(search, &data));
        FindClose(search);
    }
#else
    dir = opendir(directory);
    if (dir) {
        while ((entry = readdir(dir)) != (struct dirent *)0 &&
               catalog->count < B3D_CAMERA_PROFILE_MAX) {
            if (!b3d_camera_has_ini_extension(entry->d_name)) continue;
            if (!b3d_camera_join_path(path, sizeof(path), directory,
                                      entry->d_name)) continue;
            loaded += b3d_camera_catalog_add_path(catalog, path);
        }
        closedir(dir);
    }
#endif
    if (catalog->count <= 0) {
        blank3d_camera_catalog_init(catalog);
        b3d_camera_copy_text(catalog->directory,
                             sizeof(catalog->directory), directory);
        b3d_camera_copy_text(catalog->status, sizeof(catalog->status),
                             "camera INI directory empty; fallbacks active");
        return 0;
    }
    b3d_camera_sort(catalog);
    catalog->active_index = 0;
    catalog->loaded_from_directory = 1;
    sprintf(catalog->status, "%d camera INI profile(s) loaded", loaded);
    return loaded;
}

int blank3d_camera_catalog_select_index(Blank3DCameraCatalog *catalog,
                                        int index)
{
    if (!catalog || catalog->count <= 0) return 0;
    if (index < 0 || index >= catalog->count) return 0;
    catalog->active_index = index;
    return 1;
}

int blank3d_camera_catalog_select_id(Blank3DCameraCatalog *catalog,
                                     const char *id)
{
    int i;
    if (!catalog || !id) return 0;
    for (i = 0; i < catalog->count; ++i) {
        if (b3d_camera_text_equal(catalog->profiles[i].id, id) ||
            b3d_camera_text_equal(catalog->profiles[i].name, id)) {
            catalog->active_index = i;
            return 1;
        }
    }
    return 0;
}

int blank3d_camera_catalog_next(Blank3DCameraCatalog *catalog)
{
    if (!catalog || catalog->count <= 0) return 0;
    catalog->active_index += 1;
    if (catalog->active_index >= catalog->count) catalog->active_index = 0;
    return 1;
}

const Blank3DCameraProfile *blank3d_camera_catalog_current(
    const Blank3DCameraCatalog *catalog)
{
    if (!catalog || catalog->count <= 0 || catalog->active_index < 0 ||
        catalog->active_index >= catalog->count)
        return (const Blank3DCameraProfile *)0;
    return &catalog->profiles[catalog->active_index];
}

Blank3DCameraProfile *blank3d_camera_catalog_current_mutable(
    Blank3DCameraCatalog *catalog)
{
    if (!catalog || catalog->count <= 0 || catalog->active_index < 0 ||
        catalog->active_index >= catalog->count)
        return (Blank3DCameraProfile *)0;
    return &catalog->profiles[catalog->active_index];
}

const Blank3DCameraProfile *blank3d_camera_catalog_at(
    const Blank3DCameraCatalog *catalog,
    int index)
{
    if (!catalog || index < 0 || index >= catalog->count)
        return (const Blank3DCameraProfile *)0;
    return &catalog->profiles[index];
}

int blank3d_camera_catalog_find_view_style(
    const Blank3DCameraCatalog *catalog,
    int view_style)
{
    int i;
    if (!catalog) return -1;
    for (i = 0; i < catalog->count; ++i) {
        if (catalog->profiles[i].view_style == view_style) return i;
    }
    return -1;
}

const char *blank3d_camera_catalog_status(
    const Blank3DCameraCatalog *catalog)
{
    return catalog ? catalog->status : "camera catalog unavailable";
}
