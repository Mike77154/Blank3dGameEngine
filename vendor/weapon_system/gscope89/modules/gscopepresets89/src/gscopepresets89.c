/*
 * gscopepresets89.c
 * INI-backed vector reticle catalog.
 * C89, fixed-point only, no dynamic allocation.
 *
 * Runtime geometry is loaded from config/reticles/catalog.ini and its preset INIs.
 * The enum in the public header is retained only for legacy numeric-ID ABI.
 */
#include "../include/gscopepresets89.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#ifndef GSVP89_MAX_PRESETS
#define GSVP89_MAX_PRESETS 512
#endif
#ifndef GSVP89_MAX_SHAPES_TOTAL
#define GSVP89_MAX_SHAPES_TOTAL 8192
#endif
#ifndef GSVP89_MAX_NAME
#define GSVP89_MAX_NAME 64
#endif
#ifndef GSVP89_MAX_FAMILY_NAME
#define GSVP89_MAX_FAMILY_NAME 32
#endif
#ifndef GSVP89_MAX_PATH
#define GSVP89_MAX_PATH 320
#endif
#ifndef GSVP89_MAX_DEPTH
#define GSVP89_MAX_DEPTH 8
#endif

#define GSVP89_ERR_NONE       0
#define GSVP89_ERR_OPEN       1
#define GSVP89_ERR_PARSE      2
#define GSVP89_ERR_PRESETS    3
#define GSVP89_ERR_SHAPES     4
#define GSVP89_ERR_PATH       5
#define GSVP89_ERR_DEPTH      6
#define GSVP89_ERR_MISMATCH   7

static gsvp89_preset gsvp89_runtime_presets[GSVP89_MAX_PRESETS];
static char gsvp89_runtime_names[GSVP89_MAX_PRESETS][GSVP89_MAX_NAME];
static char gsvp89_runtime_family_names[GSVP89_MAX_PRESETS][GSVP89_MAX_FAMILY_NAME];
static gsv89_shape gsvp89_runtime_shapes[GSVP89_MAX_SHAPES_TOTAL];
static short gsvp89_runtime_count = 0;
static short gsvp89_runtime_shape_count = 0;
static short gsvp89_runtime_loaded = 0;
static short gsvp89_runtime_error = GSVP89_ERR_NONE;
static short gsvp89_runtime_error_line = 0;
static char gsvp89_catalog_path[GSVP89_MAX_PATH] = "config/reticles/catalog.ini";

static void gsvp89_copy(char *dst, int cap, const char *src)
{
    int i;
    if (!dst || cap <= 0) return;
    if (!src) src = "";
    i = 0;
    while (i + 1 < cap && src[i]) {
        dst[i] = src[i];
        ++i;
    }
    dst[i] = '\0';
}

static char *gsvp89_trim(char *s)
{
    char *e;
    while (*s && isspace((unsigned char)*s)) ++s;
    e = s + strlen(s);
    while (e > s && isspace((unsigned char)e[-1])) --e;
    *e = '\0';
    return s;
}

static int gsvp89_streq(const char *a, const char *b)
{
    if (!a || !b) return 0;
    while (*a && *b) {
        if (*a != *b) return 0;
        ++a;
        ++b;
    }
    return *a == *b;
}

static int gsvp89_is_abs(const char *p)
{
    if (!p || !p[0]) return 0;
    if (p[0] == '/' || p[0] == '\\') return 1;
    if (p[0] && p[1] == ':') return 1;
    return 0;
}

static void gsvp89_dirname(const char *path, char *out)
{
    int n;
    int i;
    int cut;
    n = (int)strlen(path);
    cut = -1;
    for (i = 0; i < n; ++i) {
        if (path[i] == '/' || path[i] == '\\') cut = i;
    }
    if (cut < 0) {
        gsvp89_copy(out, GSVP89_MAX_PATH, ".");
        return;
    }
    if (cut >= GSVP89_MAX_PATH) cut = GSVP89_MAX_PATH - 1;
    for (i = 0; i < cut; ++i) out[i] = path[i];
    out[cut] = '\0';
}

static int gsvp89_join(const char *base_file, const char *child, char *out)
{
    char dir[GSVP89_MAX_PATH];
    int need;
    if (!child || !child[0]) return 0;
    if (gsvp89_is_abs(child)) {
        if ((int)strlen(child) >= GSVP89_MAX_PATH) return 0;
        gsvp89_copy(out, GSVP89_MAX_PATH, child);
        return 1;
    }
    gsvp89_dirname(base_file, dir);
    if (!strcmp(dir, ".")) dir[0] = '\0';
    need = (int)strlen(dir) + (dir[0] ? 1 : 0) + (int)strlen(child) + 1;
    if (need > GSVP89_MAX_PATH) return 0;
    out[0] = '\0';
    if (dir[0]) {
        strcpy(out, dir);
        strcat(out, "/");
    }
    strcat(out, child);
    return 1;
}

static long gsvp89_to_long(const char *s, long fallback)
{
    char *endp;
    long v;
    if (!s || !s[0]) return fallback;
    v = strtol(s, &endp, 0);
    if (endp == s) return fallback;
    return v;
}

static short gsvp89_kind_from_name(const char *s)
{
    if (!strcmp(s,"none")) return GSV89_SHAPE_NONE;
    if (!strcmp(s,"line")) return GSV89_SHAPE_LINE;
    if (!strcmp(s,"hline")) return GSV89_SHAPE_HLINE;
    if (!strcmp(s,"vline")) return GSV89_SHAPE_VLINE;
    if (!strcmp(s,"rect")) return GSV89_SHAPE_RECT;
    if (!strcmp(s,"square")) return GSV89_SHAPE_SQUARE;
    if (!strcmp(s,"triangle")) return GSV89_SHAPE_TRIANGLE;
    if (!strcmp(s,"circle")) return GSV89_SHAPE_CIRCLE;
    if (!strcmp(s,"ellipse")) return GSV89_SHAPE_ELLIPSE;
    if (!strcmp(s,"arc")) return GSV89_SHAPE_ARC;
    if (!strcmp(s,"dot")) return GSV89_SHAPE_DOT;
    if (!strcmp(s,"cross")) return GSV89_SHAPE_CROSS;
    if (!strcmp(s,"xcross")) return GSV89_SHAPE_XCROSS;
    if (!strcmp(s,"chevron")) return GSV89_SHAPE_CHEVRON;
    if (!strcmp(s,"diamond")) return GSV89_SHAPE_DIAMOND;
    if (!strcmp(s,"parenthesis")) return GSV89_SHAPE_PARENTHESIS;
    if (!strcmp(s,"bracket")) return GSV89_SHAPE_BRACKET;
    if (!strcmp(s,"horseshoe")) return GSV89_SHAPE_HORSESHOE;
    if (!strcmp(s,"regular_polygon")) return GSV89_SHAPE_REGULAR_POLYGON;
    if (!strcmp(s,"path")) return GSV89_SHAPE_PATH;
    if (!strcmp(s,"grid")) return GSV89_SHAPE_GRID;
    if (!strcmp(s,"tick_strip")) return GSV89_SHAPE_TICK_STRIP;
    if (!strcmp(s,"bezier_quad")) return GSV89_SHAPE_BEZIER_QUAD;
    if (!strcmp(s,"glyph")) return GSV89_SHAPE_GLYPH;
    if (!strcmp(s,"scope_mask")) return GSV89_SHAPE_SCOPE_MASK;
    if (!strcmp(s,"vignette")) return GSV89_SHAPE_VIGNETTE;
    return (short)gsvp89_to_long(s, GSV89_SHAPE_NONE);
}

static short gsvp89_part_from_name(const char *s)
{
    if (!strcmp(s,"primary")) return GSV89_PART_PRIMARY;
    if (!strcmp(s,"secondary")) return GSV89_PART_SECONDARY;
    if (!strcmp(s,"center")) return GSV89_PART_CENTER;
    if (!strcmp(s,"posts")) return GSV89_PART_POSTS;
    if (!strcmp(s,"ticks")) return GSV89_PART_TICKS;
    if (!strcmp(s,"range")) return GSV89_PART_RANGE;
    if (!strcmp(s,"bdc")) return GSV89_PART_BDC;
    if (!strcmp(s,"wind")) return GSV89_PART_WIND;
    if (!strcmp(s,"lead")) return GSV89_PART_LEAD;
    if (!strcmp(s,"stadia")) return GSV89_PART_STADIA;
    if (!strcmp(s,"labels")) return GSV89_PART_LABELS;
    if (!strcmp(s,"illumination")) return GSV89_PART_ILLUMINATION;
    if (!strcmp(s,"frame")) return GSV89_PART_FRAME;
    if (!strcmp(s,"decoration")) return GSV89_PART_DECORATION;
    if (!strcmp(s,"warning")) return GSV89_PART_WARNING;
    if (!strcmp(s,"custom0")) return GSV89_PART_CUSTOM0;
    return (short)gsvp89_to_long(s, 0);
}

static short gsvp89_shape_flags_from_text(const char *src)
{
    char buf[160];
    char *tok;
    short v;
    gsvp89_copy(buf, (int)sizeof(buf), src);
    v = 0;
    tok = strtok(buf, "|");
    while (tok) {
        tok = gsvp89_trim(tok);
        if (!strcmp(tok,"filled")) v = (short)(v | GSV89_FLAG_FILLED);
        else if (!strcmp(tok,"closed")) v = (short)(v | GSV89_FLAG_CLOSED);
        else if (!strcmp(tok,"visible")) v = (short)(v | GSV89_FLAG_VISIBLE);
        else if (!strcmp(tok,"outline")) v = (short)(v | GSV89_FLAG_OUTLINE);
        else if (!strcmp(tok,"major_alt")) v = (short)(v | GSV89_FLAG_MAJOR_ALT);
        else if (!strcmp(tok,"flip")) v = (short)(v | GSV89_FLAG_FLIP);
        else v = (short)(v | (short)gsvp89_to_long(tok,0));
        tok = strtok(0, "|");
    }
    return v;
}

static short gsvp89_preset_flags_from_text(const char *src)
{
    char buf[160];
    char *tok;
    short v;
    gsvp89_copy(buf, (int)sizeof(buf), src);
    v = 0;
    tok = strtok(buf, "|");
    while (tok) {
        tok = gsvp89_trim(tok);
        if (!strcmp(tok,"pure_vector")) v = (short)(v | GSVP89_FLAG_PURE_VECTOR);
        else if (!strcmp(tok,"historical")) v = (short)(v | GSVP89_FLAG_HISTORICAL);
        else if (!strcmp(tok,"inspired")) v = (short)(v | GSVP89_FLAG_INSPIRED);
        else if (!strcmp(tok,"illuminated")) v = (short)(v | GSVP89_FLAG_ILLUMINATED);
        else if (!strcmp(tok,"launcher")) v = (short)(v | GSVP89_FLAG_LAUNCHER);
        else if (!strcmp(tok,"rangefinder")) v = (short)(v | GSVP89_FLAG_RANGEFINDER);
        else if (!strcmp(tok,"bdc")) v = (short)(v | GSVP89_FLAG_BDC);
        else v = (short)(v | (short)gsvp89_to_long(tok,0));
        tok = strtok(0, "|");
    }
    return v;
}

static short gsvp89_family_from_name(const char *s)
{
    if (!strcmp(s,"classic")) return GSVP89_FAMILY_CLASSIC;
    if (!strcmp(s,"tactical")) return GSVP89_FAMILY_TACTICAL;
    if (!strcmp(s,"historical")) return GSVP89_FAMILY_HISTORICAL;
    if (!strcmp(s,"hunting")) return GSVP89_FAMILY_HUNTING;
    if (!strcmp(s,"digital")) return GSVP89_FAMILY_DIGITAL;
    if (!strcmp(s,"launcher")) return GSVP89_FAMILY_LAUNCHER;
    return (short)gsvp89_to_long(s,0);
}

static short gsvp89_focal_from_name(const char *s)
{
    if (!strcmp(s,"fixed_screen")) return GSVP89_FOCAL_FIXED_SCREEN;
    if (!strcmp(s,"first")) return GSVP89_FOCAL_FIRST;
    if (!strcmp(s,"second")) return GSVP89_FOCAL_SECOND;
    return (short)gsvp89_to_long(s,0);
}

static short gsvp89_theme_from_name(const char *s)
{
    if (!strcmp(s,"black")) return GSVP89_THEME_BLACK;
    if (!strcmp(s,"red")) return GSVP89_THEME_RED;
    if (!strcmp(s,"green")) return GSVP89_THEME_GREEN;
    if (!strcmp(s,"amber")) return GSVP89_THEME_AMBER;
    if (!strcmp(s,"white")) return GSVP89_THEME_WHITE;
    if (!strcmp(s,"dual")) return GSVP89_THEME_DUAL;
    return (short)gsvp89_to_long(s,0);
}

static void gsvp89_shape_set(gsv89_shape *sh, const char *key, const char *value)
{
    long v;
    if (!sh || !key || !value) return;
    if (!strcmp(key,"kind")) sh->kind = gsvp89_kind_from_name(value);
    else if (!strcmp(key,"part")) sh->part_id = gsvp89_part_from_name(value);
    else if (!strcmp(key,"flags")) sh->flags = gsvp89_shape_flags_from_text(value);
    else if (!strcmp(key,"layer")) sh->layer = (short)gsvp89_to_long(value,0);
    else if (!strcmp(key,"thickness_px")) sh->thickness_px = (short)gsvp89_to_long(value,0);
    else if (!strcmp(key,"outline_px")) sh->outline_px = (short)gsvp89_to_long(value,0);
    else if (!strcmp(key,"i0")) sh->i0 = (short)gsvp89_to_long(value,0);
    else if (!strcmp(key,"i1")) sh->i1 = (short)gsvp89_to_long(value,0);
    else if (!strcmp(key,"i2")) sh->i2 = (short)gsvp89_to_long(value,0);
    else if (!strcmp(key,"i3")) sh->i3 = (short)gsvp89_to_long(value,0);
    else {
        v = gsvp89_to_long(value,0);
        if (!strcmp(key,"x0")) sh->x0 = GSP89_NORM(v);
        else if (!strcmp(key,"y0")) sh->y0 = GSP89_NORM(v);
        else if (!strcmp(key,"x1")) sh->x1 = GSP89_NORM(v);
        else if (!strcmp(key,"y1")) sh->y1 = GSP89_NORM(v);
        else if (!strcmp(key,"x2")) sh->x2 = GSP89_NORM(v);
        else if (!strcmp(key,"y2")) sh->y2 = GSP89_NORM(v);
        else if (!strcmp(key,"x3")) sh->x3 = GSP89_NORM(v);
        else if (!strcmp(key,"y3")) sh->y3 = GSP89_NORM(v);
        else if (!strcmp(key,"a")) sh->a = GSP89_NORM(v);
        else if (!strcmp(key,"b")) sh->b = GSP89_NORM(v);
        else if (!strcmp(key,"c")) sh->c = GSP89_NORM(v);
        else if (!strcmp(key,"d")) sh->d = GSP89_NORM(v);
    }
}

static void gsvp89_preset_set(gsvp89_preset *preset, short slot,
                              const char *key, const char *value,
                              short *declared_shapes)
{
    if (!preset || !key || !value) return;
    if (!strcmp(key,"name")) gsvp89_copy(gsvp89_runtime_names[slot],GSVP89_MAX_NAME,value);
    else if (!strcmp(key,"family_name")) gsvp89_copy(gsvp89_runtime_family_names[slot],GSVP89_MAX_FAMILY_NAME,value);
    else if (!strcmp(key,"family")) preset->family = gsvp89_family_from_name(value);
    else if (!strcmp(key,"flags")) preset->flags = gsvp89_preset_flags_from_text(value);
    else if (!strcmp(key,"focal_plane")) preset->focal_plane = gsvp89_focal_from_name(value);
    else if (!strcmp(key,"recommended_zoom_x100")) preset->recommended_zoom_x100 = (short)gsvp89_to_long(value,0);
    else if (!strcmp(key,"calibration_zoom_x100")) preset->calibration_zoom_x100 = (short)gsvp89_to_long(value,0);
    else if (!strcmp(key,"default_theme")) preset->default_theme = gsvp89_theme_from_name(value);
    else if (!strcmp(key,"shape_count") && declared_shapes) *declared_shapes = (short)gsvp89_to_long(value,-1);
}

static int gsvp89_load_preset_file(gsvp89_preset *preset, short slot,
                                   const char *path, short depth,
                                   short *shape_cursor, short *preset_shape_count,
                                   short *declared_shapes)
{
    FILE *fp;
    char line[384];
    char section[64];
    short line_no;
    gsv89_shape *current_shape;
    if (depth > GSVP89_MAX_DEPTH) {
        gsvp89_runtime_error = GSVP89_ERR_DEPTH;
        return 0;
    }
    fp = fopen(path,"rb");
    if (!fp) {
        gsvp89_runtime_error = GSVP89_ERR_OPEN;
        return 0;
    }
    section[0] = '\0';
    line_no = 0;
    current_shape = 0;
    while (fgets(line,(int)sizeof(line),fp)) {
        char *p;
        char *eq;
        char *key;
        char *value;
        ++line_no;
        p = gsvp89_trim(line);
        if (!p[0] || p[0]==';' || p[0]=='#') continue;
        if (p[0]=='[') {
            char *r;
            r = strchr(p+1,']');
            if (!r) {
                gsvp89_runtime_error=GSVP89_ERR_PARSE;
                gsvp89_runtime_error_line=line_no;
                fclose(fp);
                return 0;
            }
            *r='\0';
            gsvp89_copy(section,(int)sizeof(section),gsvp89_trim(p+1));
            current_shape=0;
            if (!strncmp(section,"shape",5)) {
                if (*shape_cursor >= GSVP89_MAX_SHAPES_TOTAL) {
                    gsvp89_runtime_error=GSVP89_ERR_SHAPES;
                    fclose(fp);
                    return 0;
                }
                current_shape=&gsvp89_runtime_shapes[*shape_cursor];
                memset(current_shape,0,sizeof(*current_shape));
                ++(*shape_cursor);
                ++(*preset_shape_count);
            }
            continue;
        }
        eq=strchr(p,'=');
        if (!eq) continue;
        *eq='\0';
        key=gsvp89_trim(p);
        value=gsvp89_trim(eq+1);
        if (!strcmp(section,"recipe") && !strcmp(key,"include")) {
            char full[GSVP89_MAX_PATH];
            if (!gsvp89_join(path,value,full)) {
                gsvp89_runtime_error=GSVP89_ERR_PATH;
                fclose(fp);
                return 0;
            }
            if (!gsvp89_load_preset_file(preset,slot,full,(short)(depth+1),shape_cursor,preset_shape_count,declared_shapes)) {
                fclose(fp);
                return 0;
            }
        } else if (!strcmp(section,"preset")) {
            gsvp89_preset_set(preset,slot,key,value,depth == 0 ? declared_shapes : 0);
        } else if (current_shape) {
            gsvp89_shape_set(current_shape,key,value);
        }
    }
    fclose(fp);
    return 1;
}

static int gsvp89_load_one(short slot, const char *catalog_name,
                           const char *catalog_path, const char *relative_path)
{
    char full[GSVP89_MAX_PATH];
    gsvp89_preset *preset;
    short shape_start;
    short cursor;
    short count;
    short declared;
    if (slot < 0 || slot >= GSVP89_MAX_PRESETS) return 0;
    if (!gsvp89_join(catalog_path,relative_path,full)) {
        gsvp89_runtime_error=GSVP89_ERR_PATH;
        return 0;
    }
    preset=&gsvp89_runtime_presets[slot];
    memset(preset,0,sizeof(*preset));
    gsvp89_runtime_names[slot][0]='\0';
    gsvp89_runtime_family_names[slot][0]='\0';
    shape_start=gsvp89_runtime_shape_count;
    cursor=shape_start;
    count=0;
    declared=-1;
    preset->id=slot;
    if (!gsvp89_load_preset_file(preset,slot,full,0,&cursor,&count,&declared)) return 0;
    if (!gsvp89_runtime_names[slot][0]) gsvp89_copy(gsvp89_runtime_names[slot],GSVP89_MAX_NAME,catalog_name);
    if (!gsvp89_runtime_family_names[slot][0]) gsvp89_copy(gsvp89_runtime_family_names[slot],GSVP89_MAX_FAMILY_NAME,"unknown");
    if (!gsvp89_streq(gsvp89_runtime_names[slot],catalog_name)) {
        gsvp89_runtime_error=GSVP89_ERR_MISMATCH;
        return 0;
    }
    if (declared >= 0 && declared != count) {
        gsvp89_runtime_error=GSVP89_ERR_MISMATCH;
        return 0;
    }
    preset->name=gsvp89_runtime_names[slot];
    preset->family_name=gsvp89_runtime_family_names[slot];
    preset->shapes=&gsvp89_runtime_shapes[shape_start];
    preset->shape_count=count;
    gsvp89_runtime_shape_count=cursor;
    return 1;
}

int gsvp89_reload(void)
{
    FILE *fp;
    char line[384];
    char section[64];
    short line_no;
    short expected_count;
    gsvp89_runtime_count=0;
    gsvp89_runtime_shape_count=0;
    gsvp89_runtime_loaded=0;
    gsvp89_runtime_error=GSVP89_ERR_NONE;
    gsvp89_runtime_error_line=0;
    expected_count=-1;
    fp=fopen(gsvp89_catalog_path,"rb");
    if (!fp) {
        gsvp89_runtime_error=GSVP89_ERR_OPEN;
        return 0;
    }
    section[0]='\0';
    line_no=0;
    while (fgets(line,(int)sizeof(line),fp)) {
        char *p;
        char *eq;
        char *key;
        char *value;
        ++line_no;
        p=gsvp89_trim(line);
        if (!p[0] || p[0]==';' || p[0]=='#') continue;
        if (p[0]=='[') {
            char *r=strchr(p+1,']');
            if (!r) {
                gsvp89_runtime_error=GSVP89_ERR_PARSE;
                gsvp89_runtime_error_line=line_no;
                fclose(fp);
                return 0;
            }
            *r='\0';
            gsvp89_copy(section,(int)sizeof(section),gsvp89_trim(p+1));
            continue;
        }
        eq=strchr(p,'=');
        if (!eq) continue;
        *eq='\0';
        key=gsvp89_trim(p);
        value=gsvp89_trim(eq+1);
        if (!strcmp(section,"catalog") && !strcmp(key,"count")) {
            expected_count=(short)gsvp89_to_long(value,-1);
        } else if (!strcmp(section,"presets")) {
            if (gsvp89_runtime_count >= GSVP89_MAX_PRESETS) {
                gsvp89_runtime_error=GSVP89_ERR_PRESETS;
                fclose(fp);
                return 0;
            }
            if (!gsvp89_load_one(gsvp89_runtime_count,key,gsvp89_catalog_path,value)) {
                fclose(fp);
                return 0;
            }
            ++gsvp89_runtime_count;
        }
    }
    fclose(fp);
    if (expected_count >= 0 && expected_count != gsvp89_runtime_count) {
        gsvp89_runtime_error=GSVP89_ERR_MISMATCH;
        return 0;
    }
    gsvp89_runtime_loaded=1;
    return 1;
}

void gsvp89_set_catalog_path(const char *path)
{
    if (!path || !path[0]) return;
    gsvp89_copy(gsvp89_catalog_path,GSVP89_MAX_PATH,path);
    gsvp89_runtime_loaded=0;
}

const char *gsvp89_get_catalog_path(void)
{
    return gsvp89_catalog_path;
}

short gsvp89_last_error(void)
{
    return gsvp89_runtime_error;
}

short gsvp89_last_error_line(void)
{
    return gsvp89_runtime_error_line;
}

static int gsvp89_ensure_loaded(void)
{
    if (gsvp89_runtime_loaded) return 1;
    return gsvp89_reload();
}

short gsvp89_count(void)
{
    if (!gsvp89_ensure_loaded()) return 0;
    return gsvp89_runtime_count;
}

const gsvp89_preset *gsvp89_get(short preset_id)
{
    if (!gsvp89_ensure_loaded()) return 0;
    if (preset_id < 0 || preset_id >= gsvp89_runtime_count) return 0;
    return &gsvp89_runtime_presets[preset_id];
}

const gsvp89_preset *gsvp89_find(const char *name)
{
    short i;
    if (!name || !gsvp89_ensure_loaded()) return 0;
    for (i=0;i<gsvp89_runtime_count;++i) {
        if (gsvp89_streq(name,gsvp89_runtime_presets[i].name)) return &gsvp89_runtime_presets[i];
    }
    return 0;
}

const gsvp89_preset *gsvp89_select_from_ini(const char *selector_path, const char *section_name)
{
    FILE *fp;
    char line[384];
    char section[64];
    char catalog[GSVP89_MAX_PATH];
    char use[GSVP89_MAX_NAME];
    char full_catalog[GSVP89_MAX_PATH];
    short line_no;
    if (!selector_path || !section_name) return 0;
    fp=fopen(selector_path,"rb");
    if (!fp) { gsvp89_runtime_error=GSVP89_ERR_OPEN; return 0; }
    section[0]='\0'; catalog[0]='\0'; use[0]='\0'; line_no=0;
    while (fgets(line,(int)sizeof(line),fp)) {
        char *p; char *eq; char *key; char *value;
        ++line_no; p=gsvp89_trim(line);
        if (!p[0] || p[0]==';' || p[0]=='#') continue;
        if (p[0]=='[') {
            char *r=strchr(p+1,']');
            if (!r) { fclose(fp); gsvp89_runtime_error=GSVP89_ERR_PARSE; gsvp89_runtime_error_line=line_no; return 0; }
            *r='\0'; gsvp89_copy(section,(int)sizeof(section),gsvp89_trim(p+1)); continue;
        }
        eq=strchr(p,'='); if (!eq) continue; *eq='\0'; key=gsvp89_trim(p); value=gsvp89_trim(eq+1);
        if (!strcmp(section,section_name)) {
            if (!strcmp(key,"catalog")) gsvp89_copy(catalog,GSVP89_MAX_PATH,value);
            else if (!strcmp(key,"use")) gsvp89_copy(use,GSVP89_MAX_NAME,value);
        }
    }
    fclose(fp);
    if (!catalog[0] || !use[0]) { gsvp89_runtime_error=GSVP89_ERR_PARSE; return 0; }
    if (!gsvp89_join(selector_path,catalog,full_catalog)) { gsvp89_runtime_error=GSVP89_ERR_PATH; return 0; }
    gsvp89_set_catalog_path(full_catalog);
    if (!gsvp89_reload()) return 0;
    return gsvp89_find(use);
}

static void gsvp89_theme_color(short theme,
                               gsp89_color *main_color,
                               gsp89_color *outline_color,
                               short *outline_px)
{
    if (!main_color || !outline_color || !outline_px) return;
    *outline_color = gsp89_rgba(0, 0, 0, 255);
    *outline_px = 1;
    switch (theme) {
        case GSVP89_THEME_RED:
            *main_color = gsp89_rgba(255, 36, 36, 255);
            break;
        case GSVP89_THEME_GREEN:
            *main_color = gsp89_rgba(48, 255, 96, 255);
            break;
        case GSVP89_THEME_AMBER:
            *main_color = gsp89_rgba(255, 176, 40, 255);
            break;
        case GSVP89_THEME_WHITE:
            *main_color = gsp89_rgba(255, 255, 255, 255);
            break;
        case GSVP89_THEME_DUAL:
            *main_color = gsp89_rgba(24, 24, 24, 255);
            break;
        default:
            *main_color = gsp89_rgba(24, 24, 24, 255);
            *outline_color = gsp89_rgba(255, 255, 255, 0);
            *outline_px = 0;
            break;
    }
}

void gsvp89_default_palette(const gsvp89_preset *preset,
                            gsv89_palette *out_palette)
{
    short i;
    short outline_px;
    short theme;
    gsp89_color main_color;
    gsp89_color outline_color;
    gsp89_color illumination;
    gsp89_color warning;

    if (!out_palette) return;
    gsv89_palette_init(out_palette);
    theme = preset ? preset->default_theme : GSVP89_THEME_BLACK;
    gsvp89_theme_color(theme, &main_color, &outline_color, &outline_px);
    illumination = gsp89_rgba(255, 36, 36, 255);
    warning = gsp89_rgba(255, 176, 40, 255);

    for (i = 0; i < GSV89_MAX_PARTS; ++i) {
        gsv89_palette_set_part(out_palette, i,
                               main_color, outline_color,
                               1, outline_px, 10,
                               GSP89_BLEND_ALPHA,
                               outline_px > 0 ? GSP89_FLAG_OUTLINE : 0,
                               1);
    }

    gsv89_palette_set_part(out_palette, GSV89_PART_POSTS,
                           main_color, outline_color,
                           3, outline_px, 8,
                           GSP89_BLEND_ALPHA,
                           outline_px > 0 ? GSP89_FLAG_OUTLINE : 0, 1);
    gsv89_palette_set_part(out_palette, GSV89_PART_CENTER,
                           main_color, outline_color,
                           1, outline_px, 15,
                           GSP89_BLEND_ALPHA,
                           outline_px > 0 ? GSP89_FLAG_OUTLINE : 0, 1);
    gsv89_palette_set_part(out_palette, GSV89_PART_ILLUMINATION,
                           illumination, gsp89_rgba(0, 0, 0, 255),
                           2, 1, 16,
                           GSP89_BLEND_ALPHA,
                           GSP89_FLAG_OUTLINE, 1);
    gsv89_palette_set_part(out_palette, GSV89_PART_WARNING,
                           warning, gsp89_rgba(0, 0, 0, 255),
                           1, 1, 18,
                           GSP89_BLEND_ALPHA,
                           GSP89_FLAG_OUTLINE, 1);
    gsv89_palette_set_part(out_palette, GSV89_PART_LABELS,
                           main_color, outline_color,
                           1, outline_px, 20,
                           GSP89_BLEND_ALPHA,
                           outline_px > 0 ? GSP89_FLAG_OUTLINE : 0, 1);
}

void gsvp89_emit(gsp89_painter *painter,
                 const gsvp89_preset *preset,
                 const gsv89_palette *palette_override,
                 short global_alpha)
{
    gsv89_palette local_palette;
    const gsv89_palette *palette;
    if (!painter || !preset || !preset->shapes || preset->shape_count < 1) return;
    if (palette_override) palette = palette_override;
    else {
        gsvp89_default_palette(preset, &local_palette);
        palette = &local_palette;
    }
    gsv89_emit_shapes(painter, preset->shapes, preset->shape_count,
                      palette, global_alpha);
}
