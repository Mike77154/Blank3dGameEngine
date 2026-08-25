#ifndef GAMEVERBS89_H
#define GAMEVERBS89_H

/*
 * gameverbs89 - tiny provider-driven named verb bus.
 * ISO C89, fixed-capacity, no heap, no float/double.
 *
 * The bus owns no gameplay state. It only maps textual vocabulary to
 * caller/provider behavior so independent DSLs can share verbs.
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef GVERB89_MAX_ENTRIES
#define GVERB89_MAX_ENTRIES 128
#endif

#ifndef GVERB89_NAME_CAP
#define GVERB89_NAME_CAP 48
#endif

#define GVERB89_UNHANDLED 0
#define GVERB89_HANDLED   1
#define GVERB89_ERROR    -1

#define GVERB89_KIND_CONDITION 1
#define GVERB89_KIND_ACTION    2

typedef struct gverb89_call {
    unsigned long owner;
    void *subject;
    const char *name;
    long value_q16;
    const char *value_text;
    int has_value;
    const char **argv;
    int argc;
} gverb89_call;

typedef struct gverb89_result {
    int truth;
    long value_q16;
    int instance_id;
} gverb89_result;

typedef int (*gverb89_condition_fn)(void *user, const gverb89_call *call,
                                  gverb89_result *out);
typedef int (*gverb89_action_fn)(void *user, const gverb89_call *call);

typedef struct gverb89_entry {
    int used;
    int kind;
    char name[GVERB89_NAME_CAP];
    gverb89_condition_fn condition;
    gverb89_action_fn action;
    void *user;
} gverb89_entry;

typedef struct gverb89_registry {
    gverb89_entry entries[GVERB89_MAX_ENTRIES];
    int count;
} gverb89_registry;

void gverb89_init(gverb89_registry *registry);
void gverb89_clear(gverb89_registry *registry);
int gverb89_register_condition(gverb89_registry *registry, const char *name,
                            gverb89_condition_fn fn, void *user);
int gverb89_register_action(gverb89_registry *registry, const char *name,
                         gverb89_action_fn fn, void *user);
int gverb89_has(const gverb89_registry *registry, int kind, const char *name);
int gverb89_query(gverb89_registry *registry, const gverb89_call *call,
               gverb89_result *out);
int gverb89_perform(gverb89_registry *registry, const gverb89_call *call);
int gverb89_count(const gverb89_registry *registry, int kind);
const char *gverb89_name_at(const gverb89_registry *registry, int kind, int ordinal);

#ifdef __cplusplus
}
#endif
#endif
