/*
 * gscopeini89.h - tiny recursive INI recipe resolver.
 * C89, no heap, no malloc/realloc/free, static caller-owned storage.
 */
#ifndef GSCOPEINI89_H
#define GSCOPEINI89_H

#ifdef __cplusplus
extern "C" {
#endif

#define GRI89_MAX_ENTRIES 256
#define GRI89_MAX_SECTION 32
#define GRI89_MAX_KEY 40
#define GRI89_MAX_VALUE 160
#define GRI89_MAX_PATH 256
#define GRI89_MAX_DEPTH 8
#define GRI89_MAX_SELECTS 32

typedef struct gri89_entry {
    char section[GRI89_MAX_SECTION];
    char key[GRI89_MAX_KEY];
    char value[GRI89_MAX_VALUE];
} gri89_entry;

typedef struct gri89_doc {
    gri89_entry entries[GRI89_MAX_ENTRIES];
    short count;
    short overflowed;
    short error_line;
    short error_code;
} gri89_doc;

#define GRI89_OK 0
#define GRI89_ERR_OPEN 1
#define GRI89_ERR_DEPTH 2
#define GRI89_ERR_OVERFLOW 3
#define GRI89_ERR_PARSE 4
#define GRI89_ERR_SELECT 5
#define GRI89_ERR_PATH 6

void gri89_init(gri89_doc *doc);
int gri89_load(gri89_doc *doc, const char *path);
const char *gri89_get(const gri89_doc *doc,
                      const char *section,
                      const char *key,
                      const char *fallback);
long gri89_get_long(const gri89_doc *doc,
                    const char *section,
                    const char *key,
                    long fallback);
short gri89_get_bool(const gri89_doc *doc,
                     const char *section,
                     const char *key,
                     short fallback);

#ifdef __cplusplus
}
#endif

#endif
