#include "../include/gscopeini89.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

typedef struct gri89_select {
    char section[GRI89_MAX_SECTION];
} gri89_select;

static void gri89_copy(char *dst, int cap, const char *src)
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

static char *gri89_trim(char *s)
{
    char *e;
    while (*s && isspace((unsigned char)*s)) ++s;
    e = s + strlen(s);
    while (e > s && isspace((unsigned char)e[-1])) --e;
    *e = '\0';
    return s;
}

static int gri89_is_abs(const char *p)
{
    if (!p || !p[0]) return 0;
    if (p[0] == '/' || p[0] == '\\') return 1;
    if (p[0] && p[1] == ':') return 1;
    return 0;
}

static void gri89_dirname(const char *path, char *out)
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
        gri89_copy(out, GRI89_MAX_PATH, ".");
        return;
    }
    if (cut >= GRI89_MAX_PATH) cut = GRI89_MAX_PATH - 1;
    for (i = 0; i < cut; ++i) out[i] = path[i];
    out[cut] = '\0';
}

static int gri89_join(const char *base_file, const char *child, char *out)
{
    char dir[GRI89_MAX_PATH];
    int need;
    if (gri89_is_abs(child)) {
        if ((int)strlen(child) >= GRI89_MAX_PATH) return 0;
        gri89_copy(out, GRI89_MAX_PATH, child);
        return 1;
    }
    gri89_dirname(base_file, dir);
    need = (int)strlen(dir) + 1 + (int)strlen(child) + 1;
    if (need > GRI89_MAX_PATH) return 0;
    strcpy(out, dir);
    if (dir[0] && strcmp(dir, ".") != 0) strcat(out, "/");
    else if (strcmp(dir, ".") == 0) {
        out[0] = '\0';
    }
    strcat(out, child);
    return 1;
}

static int gri89_set(gri89_doc *doc,
                     const char *section,
                     const char *key,
                     const char *value)
{
    short i;
    for (i = 0; i < doc->count; ++i) {
        if (strcmp(doc->entries[i].section, section) == 0 &&
            strcmp(doc->entries[i].key, key) == 0) {
            gri89_copy(doc->entries[i].value, GRI89_MAX_VALUE, value);
            return 1;
        }
    }
    if (doc->count >= GRI89_MAX_ENTRIES) {
        doc->overflowed = 1;
        doc->error_code = GRI89_ERR_OVERFLOW;
        return 0;
    }
    gri89_copy(doc->entries[doc->count].section, GRI89_MAX_SECTION, section);
    gri89_copy(doc->entries[doc->count].key, GRI89_MAX_KEY, key);
    gri89_copy(doc->entries[doc->count].value, GRI89_MAX_VALUE, value);
    ++doc->count;
    return 1;
}

const char *gri89_get(const gri89_doc *doc,
                      const char *section,
                      const char *key,
                      const char *fallback)
{
    short i;
    if (!doc) return fallback;
    for (i = (short)(doc->count - 1); i >= 0; --i) {
        if (strcmp(doc->entries[i].section, section) == 0 &&
            strcmp(doc->entries[i].key, key) == 0)
            return doc->entries[i].value;
    }
    return fallback;
}

long gri89_get_long(const gri89_doc *doc,
                    const char *section,
                    const char *key,
                    long fallback)
{
    const char *s;
    char *endp;
    long v;
    s = gri89_get(doc, section, key, 0);
    if (!s) return fallback;
    v = strtol(s, &endp, 0);
    if (endp == s) return fallback;
    return v;
}

short gri89_get_bool(const gri89_doc *doc,
                     const char *section,
                     const char *key,
                     short fallback)
{
    const char *s;
    s = gri89_get(doc, section, key, 0);
    if (!s) return fallback;
    if (!strcmp(s, "1") || !strcmp(s, "yes") || !strcmp(s, "true") || !strcmp(s, "on")) return 1;
    if (!strcmp(s, "0") || !strcmp(s, "no") || !strcmp(s, "false") || !strcmp(s, "off")) return 0;
    return fallback;
}

static int gri89_load_internal(gri89_doc *doc, const char *path, short depth)
{
    FILE *fp;
    char line[384];
    char section[GRI89_MAX_SECTION];
    gri89_select selects[GRI89_MAX_SELECTS];
    short select_count;
    short line_no;
    if (depth > GRI89_MAX_DEPTH) {
        doc->error_code = GRI89_ERR_DEPTH;
        return 0;
    }
    fp = fopen(path, "rb");
    if (!fp) {
        doc->error_code = GRI89_ERR_OPEN;
        return 0;
    }
    section[0] = '\0';
    select_count = 0;
    line_no = 0;
    while (fgets(line, (int)sizeof(line), fp)) {
        char *p;
        char *eq;
        char *key;
        char *value;
        ++line_no;
        p = gri89_trim(line);
        if (!p[0] || p[0] == ';' || p[0] == '#') continue;
        if (p[0] == '[') {
            char *r;
            r = strchr(p + 1, ']');
            if (!r) {
                doc->error_line = line_no;
                doc->error_code = GRI89_ERR_PARSE;
                fclose(fp);
                return 0;
            }
            *r = '\0';
            gri89_copy(section, GRI89_MAX_SECTION, gri89_trim(p + 1));
            continue;
        }
        eq = strchr(p, '=');
        if (!eq) continue;
        *eq = '\0';
        key = gri89_trim(p);
        value = gri89_trim(eq + 1);
        if (!strcmp(section, "recipe") && !strcmp(key, "include")) {
            char full[GRI89_MAX_PATH];
            if (!gri89_join(path, value, full)) {
                doc->error_code = GRI89_ERR_PATH;
                fclose(fp);
                return 0;
            }
            if (!gri89_load_internal(doc, full, (short)(depth + 1))) {
                fclose(fp);
                return 0;
            }
        } else if (!strcmp(section, "recipe") && !strcmp(key, "select")) {
            if (select_count >= GRI89_MAX_SELECTS) {
                doc->error_code = GRI89_ERR_OVERFLOW;
                fclose(fp);
                return 0;
            }
            gri89_copy(selects[select_count].section, GRI89_MAX_SECTION, value);
            ++select_count;
        } else {
            if (!gri89_set(doc, section, key, value)) {
                fclose(fp);
                return 0;
            }
        }
    }
    fclose(fp);

    {
        short si;
        for (si = 0; si < select_count; ++si) {
            const char *catalog;
            const char *use;
            char catalog_path[GRI89_MAX_PATH];
            char preset_path[GRI89_MAX_PATH];
            char full_preset[GRI89_MAX_PATH];
            gri89_doc cat;
            catalog = gri89_get(doc, selects[si].section, "catalog", 0);
            use = gri89_get(doc, selects[si].section, "use", 0);
            if (!catalog || !use) {
                doc->error_code = GRI89_ERR_SELECT;
                return 0;
            }
            if (!gri89_join(path, catalog, catalog_path)) {
                doc->error_code = GRI89_ERR_PATH;
                return 0;
            }
            gri89_init(&cat);
            if (!gri89_load_internal(&cat, catalog_path, (short)(depth + 1))) {
                doc->error_code = cat.error_code;
                doc->error_line = cat.error_line;
                return 0;
            }
            gri89_copy(preset_path, GRI89_MAX_PATH,
                       gri89_get(&cat, "presets", use, ""));
            if (!preset_path[0]) {
                doc->error_code = GRI89_ERR_SELECT;
                return 0;
            }
            if (!gri89_join(catalog_path, preset_path, full_preset)) {
                doc->error_code = GRI89_ERR_PATH;
                return 0;
            }
            if (!gri89_load_internal(doc, full_preset, (short)(depth + 1)))
                return 0;
        }
    }
    return 1;
}

void gri89_init(gri89_doc *doc)
{
    if (!doc) return;
    doc->count = 0;
    doc->overflowed = 0;
    doc->error_line = 0;
    doc->error_code = GRI89_OK;
}

int gri89_load(gri89_doc *doc, const char *path)
{
    if (!doc || !path) return 0;
    return gri89_load_internal(doc, path, 0);
}
