#include <stdio.h>
#include "gskybox89_assets.h"

static int gasset_is_slash(char c)
{
    return c == '/' || c == '\\';
}

static int gasset_tolower_i(int c)
{
    if (c >= 'A' && c <= 'Z') return c + ('a' - 'A');
    return c;
}

static int gasset_streq_ci(const char *a, const char *b)
{
    int ca;
    int cb;
    if (a == 0 || b == 0) return 0;
    while (*a != '\0' && *b != '\0') {
        ca = gasset_tolower_i((unsigned char)*a);
        cb = gasset_tolower_i((unsigned char)*b);
        if (ca != cb) return 0;
        ++a;
        ++b;
    }
    return *a == '\0' && *b == '\0';
}

static int gasset_endswith_ci(const char *s, const char *suffix)
{
    int sl;
    int tl;
    int i;
    if (s == 0 || suffix == 0) return 0;
    sl = 0;
    tl = 0;
    while (s[sl] != '\0') ++sl;
    while (suffix[tl] != '\0') ++tl;
    if (tl > sl) return 0;
    for (i = 0; i < tl; ++i) {
        if (gasset_tolower_i((unsigned char)s[sl - tl + i]) != gasset_tolower_i((unsigned char)suffix[i])) return 0;
    }
    return 1;
}

static int gasset_contains_ci(const char *s, const char *needle)
{
    int i;
    int j;
    int ok;
    if (s == 0 || needle == 0) return 0;
    if (needle[0] == '\0') return 1;
    for (i = 0; s[i] != '\0'; ++i) {
        ok = 1;
        for (j = 0; needle[j] != '\0'; ++j) {
            if (s[i + j] == '\0') { ok = 0; break; }
            if (gasset_tolower_i((unsigned char)s[i + j]) != gasset_tolower_i((unsigned char)needle[j])) { ok = 0; break; }
        }
        if (ok != 0) return 1;
    }
    return 0;
}

static const char *gasset_basename(const char *path)
{
    const char *b;
    if (path == 0) return 0;
    b = path;
    while (*path != '\0') {
        if (gasset_is_slash(*path)) b = path + 1;
        ++path;
    }
    return b;
}

static void gasset_copy(char *dst, int dst_cap, const char *src)
{
    int i;
    if (dst == 0 || dst_cap <= 0) return;
    if (src == 0) {
        dst[0] = '\0';
        return;
    }
    i = 0;
    while (i < dst_cap - 1 && src[i] != '\0') {
        dst[i] = src[i];
        ++i;
    }
    dst[i] = '\0';
}

static void gasset_name_without_ext(const char *path, char *out_name, int out_cap)
{
    const char *b;
    int i;
    int last_dot;
    if (out_name == 0 || out_cap <= 0) return;
    out_name[0] = '\0';
    b = gasset_basename(path);
    if (b == 0) return;
    i = 0;
    last_dot = -1;
    while (i < out_cap - 1 && b[i] != '\0') {
        out_name[i] = b[i];
        if (b[i] == '.') last_dot = i;
        ++i;
    }
    out_name[i] = '\0';
    if (last_dot >= 0) out_name[last_dot] = '\0';
}


static void gasset_strip_trailing_digits(char *s)
{
    int n;
    if (s == 0) return;
    n = 0;
    while (s[n] != '\0') ++n;
    while (n > 0 && s[n - 1] >= '0' && s[n - 1] <= '9') {
        s[n - 1] = '\0';
        --n;
    }
}

static int gasset_has_supported_ext(const char *path, int flags)
{
    if ((flags & GSKYBOX89_ASSET_FLAG_ALLOW_IMAGES) != 0) {
        if (gasset_endswith_ci(path, ".tga")) return 1;
        if (gasset_endswith_ci(path, ".bmp")) return 1;
        if (gasset_endswith_ci(path, ".ppm")) return 1;
        if (gasset_endswith_ci(path, ".png")) return 1; /* mapper only; decoder is backend/tool-specific */
        if (gasset_endswith_ci(path, ".jpg")) return 1;
        if (gasset_endswith_ci(path, ".jpeg")) return 1;
    }
    if ((flags & GSKYBOX89_ASSET_FLAG_ALLOW_VMT) != 0) {
        if (gasset_endswith_ci(path, ".vmt")) return 1;
    }
    if ((flags & GSKYBOX89_ASSET_FLAG_ALLOW_VTF) != 0) {
        if (gasset_endswith_ci(path, ".vtf")) return 1;
    }
    return 0;
}

static int gasset_suffix_source(const char *name, int *out_face)
{
    if (gasset_endswith_ci(name, "rt")) { *out_face = GSKYBOX89_FACE_POS_X; return 1; }
    if (gasset_endswith_ci(name, "lf")) { *out_face = GSKYBOX89_FACE_NEG_X; return 1; }
    if (gasset_endswith_ci(name, "up")) { *out_face = GSKYBOX89_FACE_POS_Y; return 1; }
    if (gasset_endswith_ci(name, "dn")) { *out_face = GSKYBOX89_FACE_NEG_Y; return 1; }
    if (gasset_endswith_ci(name, "ft")) { *out_face = GSKYBOX89_FACE_POS_Z; return 1; }
    if (gasset_endswith_ci(name, "bk")) { *out_face = GSKYBOX89_FACE_NEG_Z; return 1; }
    return 0;
}

static int gasset_suffix_axis(const char *name, int *out_face)
{
    if (gasset_streq_ci(name, "px") || gasset_endswith_ci(name, "_px") || gasset_endswith_ci(name, "-px")) { *out_face = GSKYBOX89_FACE_POS_X; return 1; }
    if (gasset_streq_ci(name, "nx") || gasset_endswith_ci(name, "_nx") || gasset_endswith_ci(name, "-nx")) { *out_face = GSKYBOX89_FACE_NEG_X; return 1; }
    if (gasset_streq_ci(name, "py") || gasset_endswith_ci(name, "_py") || gasset_endswith_ci(name, "-py")) { *out_face = GSKYBOX89_FACE_POS_Y; return 1; }
    if (gasset_streq_ci(name, "ny") || gasset_endswith_ci(name, "_ny") || gasset_endswith_ci(name, "-ny")) { *out_face = GSKYBOX89_FACE_NEG_Y; return 1; }
    if (gasset_streq_ci(name, "pz") || gasset_endswith_ci(name, "_pz") || gasset_endswith_ci(name, "-pz")) { *out_face = GSKYBOX89_FACE_POS_Z; return 1; }
    if (gasset_streq_ci(name, "nz") || gasset_endswith_ci(name, "_nz") || gasset_endswith_ci(name, "-nz")) { *out_face = GSKYBOX89_FACE_NEG_Z; return 1; }
    return 0;
}

static int gasset_suffix_word(const char *name, int *out_face)
{
    if (gasset_endswith_ci(name, "right") || gasset_endswith_ci(name, "_right")) { *out_face = GSKYBOX89_FACE_POS_X; return 1; }
    if (gasset_endswith_ci(name, "left") || gasset_endswith_ci(name, "_left")) { *out_face = GSKYBOX89_FACE_NEG_X; return 1; }
    if (gasset_endswith_ci(name, "top") || gasset_endswith_ci(name, "_top") || gasset_endswith_ci(name, "up") || gasset_endswith_ci(name, "_up")) { *out_face = GSKYBOX89_FACE_POS_Y; return 1; }
    if (gasset_endswith_ci(name, "bottom") || gasset_endswith_ci(name, "_bottom") || gasset_endswith_ci(name, "down") || gasset_endswith_ci(name, "_down")) { *out_face = GSKYBOX89_FACE_NEG_Y; return 1; }
    if (gasset_endswith_ci(name, "front") || gasset_endswith_ci(name, "_front")) { *out_face = GSKYBOX89_FACE_POS_Z; return 1; }
    if (gasset_endswith_ci(name, "back") || gasset_endswith_ci(name, "_back")) { *out_face = GSKYBOX89_FACE_NEG_Z; return 1; }
    return 0;
}

void gskybox89_asset_set_clear(Gskybox89_AssetSet *set)
{
    int i;
    if (set == 0) return;
    for (i = 0; i < GSKYBOX89_CUBE_FACE_COUNT; ++i) {
        set->face_path[i][0] = '\0';
        set->face_key[i][0] = '\0';
        set->present[i] = 0;
        set->convention[i] = GSKYBOX89_ASSET_CONV_UNKNOWN;
    }
    set->present_count = 0;
}

int gskybox89_asset_face_from_name(const char *path_or_name, int *out_face, int *out_convention)
{
    char name[GSKYBOX89_ASSET_KEY_MAX];
    char stripped[GSKYBOX89_ASSET_KEY_MAX];
    int face;
    if (path_or_name == 0 || out_face == 0) return GSKYBOX89_ASSET_ERR_NULL;
    gasset_name_without_ext(path_or_name, name, (int)sizeof(name));
    if (name[0] == '\0') return GSKYBOX89_ASSET_ERR_NOT_FOUND;
    gasset_copy(stripped, (int)sizeof(stripped), name);
    gasset_strip_trailing_digits(stripped);
    face = GSKYBOX89_FACE_NONE;
    if (gasset_suffix_axis(name, &face) != 0 || gasset_suffix_axis(stripped, &face) != 0) {
        *out_face = face;
        if (out_convention != 0) *out_convention = GSKYBOX89_ASSET_CONV_AXIS6;
        return GSKYBOX89_ASSET_OK;
    }
    if (gasset_suffix_source(name, &face) != 0 || gasset_suffix_source(stripped, &face) != 0) {
        *out_face = face;
        if (out_convention != 0) *out_convention = GSKYBOX89_ASSET_CONV_SOURCE6;
        return GSKYBOX89_ASSET_OK;
    }
    if (gasset_suffix_word(name, &face) != 0 || gasset_suffix_word(stripped, &face) != 0) {
        *out_face = face;
        if (out_convention != 0) *out_convention = GSKYBOX89_ASSET_CONV_WORD6;
        return GSKYBOX89_ASSET_OK;
    }
    return GSKYBOX89_ASSET_ERR_NOT_FOUND;
}

int gskybox89_asset_add_path(Gskybox89_AssetSet *set, const char *path, int flags)
{
    int face;
    int conv;
    int rc;
    char key[GSKYBOX89_ASSET_KEY_MAX];
    if (set == 0 || path == 0) return GSKYBOX89_ASSET_ERR_NULL;
    if (flags == 0) flags = GSKYBOX89_ASSET_FLAG_DEFAULT;
    if (gasset_has_supported_ext(path, flags) == 0) return GSKYBOX89_ASSET_ERR_NOT_FOUND;
    face = GSKYBOX89_FACE_NONE;
    conv = GSKYBOX89_ASSET_CONV_UNKNOWN;
    rc = gskybox89_asset_face_from_name(path, &face, &conv);
    if (rc != GSKYBOX89_ASSET_OK) return rc;
    if (face < 0 || face >= GSKYBOX89_CUBE_FACE_COUNT) return GSKYBOX89_ASSET_ERR_RANGE;
    if (set->present[face] == 0) set->present_count += 1;
    set->present[face] = 1;
    set->convention[face] = conv;
    gasset_copy(set->face_path[face], GSKYBOX89_ASSET_PATH_MAX, path);
    gasset_name_without_ext(path, key, (int)sizeof(key));
    gasset_copy(set->face_key[face], GSKYBOX89_ASSET_KEY_MAX, key);
    return face;
}

int gskybox89_asset_complete_mask(const Gskybox89_AssetSet *set)
{
    int i;
    int mask;
    if (set == 0) return 0;
    mask = 0;
    for (i = 0; i < GSKYBOX89_CUBE_FACE_COUNT; ++i) {
        if (set->present[i] != 0) mask |= (1 << i);
    }
    return mask;
}

const char *gskybox89_asset_convention_name(int convention)
{
    if (convention == GSKYBOX89_ASSET_CONV_SOURCE6) return "Source6 FT/BK/LF/RT/UP/DN";
    if (convention == GSKYBOX89_ASSET_CONV_AXIS6) return "Axis6 px/nx/py/ny/pz/nz";
    if (convention == GSKYBOX89_ASSET_CONV_WORD6) return "Word6 right/left/up/down/front/back";
    return "unknown";
}

const char *gskybox89_asset_source_suffix_for_face(int face)
{
    if (face == GSKYBOX89_FACE_POS_X) return "RT";
    if (face == GSKYBOX89_FACE_NEG_X) return "LF";
    if (face == GSKYBOX89_FACE_POS_Y) return "UP";
    if (face == GSKYBOX89_FACE_NEG_Y) return "DN";
    if (face == GSKYBOX89_FACE_POS_Z) return "FT";
    if (face == GSKYBOX89_FACE_NEG_Z) return "BK";
    return "??";
}

const char *gskybox89_asset_axis_suffix_for_face(int face)
{
    if (face == GSKYBOX89_FACE_POS_X) return "px";
    if (face == GSKYBOX89_FACE_NEG_X) return "nx";
    if (face == GSKYBOX89_FACE_POS_Y) return "py";
    if (face == GSKYBOX89_FACE_NEG_Y) return "ny";
    if (face == GSKYBOX89_FACE_POS_Z) return "pz";
    if (face == GSKYBOX89_FACE_NEG_Z) return "nz";
    return "??";
}

static int gasset_copy_second_quoted_value(const char *line, char *out, int out_cap)
{
    int i;
    int quote_count;
    int start;
    int end;
    if (line == 0 || out == 0 || out_cap <= 0) return GSKYBOX89_ASSET_ERR_NULL;
    out[0] = '\0';
    quote_count = 0;
    start = -1;
    end = -1;
    for (i = 0; line[i] != '\0'; ++i) {
        if (line[i] == '"') {
            ++quote_count;
            if (quote_count == 3) start = i + 1;
            else if (quote_count == 4) { end = i; break; }
        }
    }
    if (start < 0 || end < start) return GSKYBOX89_ASSET_ERR_NOT_FOUND;
    i = 0;
    while (start + i < end && i < out_cap - 1) {
        out[i] = line[start + i];
        ++i;
    }
    out[i] = '\0';
    return GSKYBOX89_ASSET_OK;
}

int gskybox89_asset_vmf_extract_skyname(const char *vmf_path, char *out_skyname, int out_cap)
{
    FILE *f;
    char line[512];
    int rc;
    if (vmf_path == 0 || out_skyname == 0 || out_cap <= 0) return GSKYBOX89_ASSET_ERR_NULL;
    out_skyname[0] = '\0';
    f = fopen(vmf_path, "rb");
    if (f == 0) return GSKYBOX89_ASSET_ERR_IO;
    while (fgets(line, (int)sizeof(line), f) != 0) {
        if (gasset_contains_ci(line, "\"skyname\"") != 0) {
            rc = gasset_copy_second_quoted_value(line, out_skyname, out_cap);
            fclose(f);
            return rc;
        }
    }
    fclose(f);
    return GSKYBOX89_ASSET_ERR_NOT_FOUND;
}

int gskybox89_asset_vmt_extract_basetexture(const char *vmt_path, char *out_basetexture, int out_cap)
{
    FILE *f;
    char line[512];
    int rc;
    if (vmt_path == 0 || out_basetexture == 0 || out_cap <= 0) return GSKYBOX89_ASSET_ERR_NULL;
    out_basetexture[0] = '\0';
    f = fopen(vmt_path, "rb");
    if (f == 0) return GSKYBOX89_ASSET_ERR_IO;
    while (fgets(line, (int)sizeof(line), f) != 0) {
        if (gasset_contains_ci(line, "$basetexture") != 0) {
            rc = gasset_copy_second_quoted_value(line, out_basetexture, out_cap);
            fclose(f);
            return rc;
        }
    }
    fclose(f);
    return GSKYBOX89_ASSET_ERR_NOT_FOUND;
}

void gskybox89_asset_apply_uv_preset(Gskybox89_Config *cfg, int convention)
{
    int all;
    if (cfg == 0) return;
    all = GSKYBOX89_CUBE_ALL_FACES;
    /*
     * Keep default conservative: most pre-split Source TGA/BMP and axis PNG
     * sets are already authored top-left in their face image. Renderer authors
     * can still flip/swap individual faces through gskybox89_cube_set_uv_xform.
     */
    if (convention == GSKYBOX89_ASSET_CONV_SOURCE6) {
        gskybox89_cube_set_uv_xform(cfg, 0, 0, 0);
    } else if (convention == GSKYBOX89_ASSET_CONV_AXIS6) {
        gskybox89_cube_set_uv_xform(cfg, 0, 0, 0);
    } else if (convention == GSKYBOX89_ASSET_CONV_WORD6) {
        gskybox89_cube_set_uv_xform(cfg, 0, 0, 0);
    } else {
        gskybox89_cube_set_uv_xform(cfg, 0, 0, 0);
    }
    (void)all;
}
