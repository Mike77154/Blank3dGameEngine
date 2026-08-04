#ifndef NUMSYS_H
#define NUMSYS_H

/*
    NUMSYS C89
    Generic numeric systems for games and simulations.

    Contract:
    - C89-friendly API.
    - No dynamic allocation owned by the library.
    - No console I/O dependency in the library.
    - No decimal machine-number types in the public API or implementation.
    - All numeric gameplay values use signed fixed-point ns_fx.

    Design lineage:
    - GameMaker-style simple named values and clamp behavior.
    - Construct-style instance values, families/templates and event actions.
    - Fusion-style alterable/global values and counters/bindings.
    - Attribute-system-style base/current values and fixed-point modifiers.
*/

#ifdef __cplusplus
extern "C" {
#endif

#ifndef NS_MAX_TYPES
#define NS_MAX_TYPES 96
#endif

#ifndef NS_MAX_VALUES
#define NS_MAX_VALUES 768
#endif

#ifndef NS_MAX_THRESHOLDS
#define NS_MAX_THRESHOLDS 768
#endif

#ifndef NS_MAX_EVENTS
#define NS_MAX_EVENTS 768
#endif

#ifndef NS_MAX_TEMPLATES
#define NS_MAX_TEMPLATES 64
#endif

#ifndef NS_MAX_TEMPLATE_ITEMS
#define NS_MAX_TEMPLATE_ITEMS 512
#endif

#ifndef NS_MAX_MODIFIERS
#define NS_MAX_MODIFIERS 768
#endif

#ifndef NS_MAX_DERIVED
#define NS_MAX_DERIVED 256
#endif

#ifndef NS_MAX_BINDINGS
#define NS_MAX_BINDINGS 256
#endif

#ifndef NS_NAME_MAX
#define NS_NAME_MAX 31
#endif

#define NS_VERSION_MAJOR 0
#define NS_VERSION_MINOR 3
#define NS_VERSION_PATCH 0

#define NS_FALSE 0
#define NS_TRUE 1

#define NS_OK 0
#define NS_ERR_NULL -1
#define NS_ERR_FULL -2
#define NS_ERR_NOT_FOUND -3
#define NS_ERR_BAD_ID -4
#define NS_ERR_BAD_ARG -5
#define NS_ERR_RANGE -6
#define NS_ERR_LOCKED -7

#define NS_INVALID_ID -1
#define NS_OWNER_GLOBAL 0

/*
    Fixed-point format: 1.0 is NS_FX_ONE.
    Default precision is 1/1024.

    Examples:
    NS_FX_FROM_INT(100)      -> 100.0 units
    NS_FX_FROM_RATIO(1, 4)   -> 0.25 units
    NS_FX_FROM_PERCENT(20)   -> 0.20 ratio, useful for modifiers
*/
typedef long ns_fx;
typedef int ns_id;
typedef int ns_owner;

#define NS_FX_ONE 1024L
#define NS_FX_ZERO 0L
#define NS_FX_HALF (NS_FX_ONE / 2L)
#define NS_FX_FROM_INT(x) ((ns_fx)(x) * NS_FX_ONE)
#define NS_FX_TO_INT(x) ((int)((x) / NS_FX_ONE))
#define NS_FX_FROM_RATIO(num, den) (((ns_fx)(num) * NS_FX_ONE) / (den))
#define NS_FX_FROM_PERCENT(percent) (((ns_fx)(percent) * NS_FX_ONE) / 100L)
#define NS_FX_TO_PERCENT_INT(x) ((int)(((x) * 100L) / NS_FX_ONE))
#define NS_FX_PERCENT_100 NS_FX_FROM_INT(100)

#define NS_MOD_FOREVER -1

enum {
    NS_SCOPE_GLOBAL = 1,
    NS_SCOPE_FRAME = 2,
    NS_SCOPE_INSTANCE = 3,
    NS_SCOPE_FAMILY = 4,
    NS_SCOPE_LOCAL = 5,
    NS_SCOPE_COMPONENT = 6
};

enum {
    NS_KIND_FIXED = 1,
    NS_KIND_INT = 2,
    NS_KIND_BOOL = 3
};

enum {
    NS_OVERFLOW_NONE = 0,
    NS_OVERFLOW_CLAMP = 1,
    NS_OVERFLOW_WRAP = 2,
    NS_OVERFLOW_BOUNCE = 3,
    NS_OVERFLOW_SPILL = 4
};

enum {
    NS_CMP_EQ = 1,
    NS_CMP_NE = 2,
    NS_CMP_LT = 3,
    NS_CMP_LTE = 4,
    NS_CMP_GT = 5,
    NS_CMP_GTE = 6
};

enum {
    NS_EDGE_UP = 1,
    NS_EDGE_DOWN = 2,
    NS_EDGE_ANY = 3
};

enum {
    NS_EVENT_CHANGED = 1,
    NS_EVENT_INCREASED = 2,
    NS_EVENT_DECREASED = 3,
    NS_EVENT_EMPTY = 4,
    NS_EVENT_FULL = 5,
    NS_EVENT_THRESHOLD = 6,
    NS_EVENT_RESET = 7,
    NS_EVENT_MODIFIER_ADDED = 8,
    NS_EVENT_MODIFIER_REMOVED = 9,
    NS_EVENT_MODIFIER_EXPIRED = 10,
    NS_EVENT_DERIVED_UPDATED = 11
};

enum {
    NS_FLAG_NONE = 0,
    NS_FLAG_SAVE = 1,
    NS_FLAG_HUD = 2,
    NS_FLAG_LOCKED = 4,
    NS_FLAG_TAGGED = 8,
    NS_FLAG_DEBUG = 16
};

enum {
    NS_MOD_TARGET_VALUE = 1,
    NS_MOD_TARGET_MIN = 2,
    NS_MOD_TARGET_MAX = 3,
    NS_MOD_TARGET_REGEN = 4,
    NS_MOD_TARGET_DRAIN = 5
};

enum {
    NS_MOD_FLAT = 1,
    NS_MOD_PERCENT_ADD = 2,
    NS_MOD_PERCENT_MUL = 3
};

enum {
    NS_DERIVED_COPY = 1,
    NS_DERIVED_SUM = 2,
    NS_DERIVED_SUB = 3,
    NS_DERIVED_PRODUCT = 4,
    NS_DERIVED_RATIO = 5,
    NS_DERIVED_MIN = 6,
    NS_DERIVED_MAX = 7,
    NS_DERIVED_PERCENT = 8,
    NS_DERIVED_LERP = 9
};

enum {
    NS_BIND_BAR = 1,
    NS_BIND_COUNTER = 2,
    NS_BIND_ECG = 3,
    NS_BIND_ICON = 4,
    NS_BIND_DEBUG = 5,
    NS_BIND_CUSTOM = 6
};

typedef struct NS_TypeDef {
    int used;
    char name[NS_NAME_MAX + 1];
    int scope;
    int kind;
    int overflow;
    ns_fx initial_value;
    ns_fx min_value;
    ns_fx max_value;
    ns_fx regen_per_tick;
    ns_fx drain_per_tick;
    int flags;
} NS_TypeDef;

typedef struct NS_Value {
    int used;
    ns_owner owner;
    ns_id type_id;

    ns_fx base_value;
    ns_fx value;
    ns_fx previous_value;
    ns_fx initial_value;

    ns_fx base_min_value;
    ns_fx base_max_value;
    ns_fx min_value;
    ns_fx max_value;

    ns_fx base_regen_per_tick;
    ns_fx base_drain_per_tick;
    ns_fx regen_per_tick;
    ns_fx drain_per_tick;

    int overflow;
    int kind;
    int flags;
    int tag;
    int changed;
    int dirty;
    ns_id spill_target_value_id;
} NS_Value;

typedef struct NS_Threshold {
    int used;
    ns_id value_id;
    ns_fx threshold;
    int edge;
    int once;
    int fired;
    int user_code;
} NS_Threshold;

typedef struct NS_Event {
    int event_type;
    ns_id value_id;
    ns_id type_id;
    ns_owner owner;
    ns_fx previous_value;
    ns_fx value;
    ns_fx threshold;
    ns_id threshold_id;
    int user_code;
} NS_Event;

typedef struct NS_Template {
    int used;
    char name[NS_NAME_MAX + 1];
    int flags;
} NS_Template;

typedef struct NS_TemplateItem {
    int used;
    ns_id template_id;
    ns_id type_id;
} NS_TemplateItem;

typedef struct NS_Modifier {
    int used;
    ns_id value_id;
    int code;
    int target;
    int mode;
    ns_fx amount;
    int duration_ticks;
    int priority;
    int flags;
} NS_Modifier;

typedef struct NS_Derived {
    int used;
    ns_id output_value_id;
    ns_id a_value_id;
    ns_id b_value_id;
    int mode;
    ns_fx scale;
    ns_fx offset;
    int flags;
} NS_Derived;

typedef struct NS_Binding {
    int used;
    ns_id value_id;
    int kind;
    int user_code;
    int channel;
    int flags;
} NS_Binding;

typedef struct NS_Query {
    int use_owner;
    ns_owner owner;
    int use_type;
    ns_id type_id;
    int use_cmp;
    int cmp;
    ns_fx rhs;
    int use_tag;
    int tag;
    int flags_all;
    int flags_any;
} NS_Query;

typedef struct NS_PackItem {
    const char *name;
    int scope;
    int kind;
    ns_fx initial_value;
    ns_fx min_value;
    ns_fx max_value;
    int overflow;
    int flags;
    ns_fx regen_per_tick;
    ns_fx drain_per_tick;
} NS_PackItem;

typedef struct NS_SnapshotValue {
    int used;
    ns_owner owner;
    char type_name[NS_NAME_MAX + 1];
    ns_fx base_value;
    ns_fx initial_value;
    ns_fx base_min_value;
    ns_fx base_max_value;
    ns_fx base_regen_per_tick;
    ns_fx base_drain_per_tick;
    int overflow;
    int kind;
    int flags;
    int tag;
} NS_SnapshotValue;

struct NS_World;
typedef void (*NS_EventProc)(struct NS_World *world, const NS_Event *event_data, void *user_data);
typedef int (*NS_WriteProc)(void *user_data, const void *data, unsigned int size);
typedef int (*NS_ReadProc)(void *user_data, void *data, unsigned int size);

typedef struct NS_World {
    NS_TypeDef types[NS_MAX_TYPES];
    NS_Value values[NS_MAX_VALUES];
    NS_Threshold thresholds[NS_MAX_THRESHOLDS];
    NS_Event events[NS_MAX_EVENTS];
    NS_Template templates[NS_MAX_TEMPLATES];
    NS_TemplateItem template_items[NS_MAX_TEMPLATE_ITEMS];
    NS_Modifier modifiers[NS_MAX_MODIFIERS];
    NS_Derived derived[NS_MAX_DERIVED];
    NS_Binding bindings[NS_MAX_BINDINGS];
    int event_count;
    NS_EventProc callback;
    void *callback_user_data;
} NS_World;

ns_fx ns_fx_mul(ns_fx a, ns_fx b);
ns_fx ns_fx_div(ns_fx a, ns_fx b);
ns_fx ns_fx_clamp(ns_fx value, ns_fx min_value, ns_fx max_value);
ns_fx ns_fx_lerp(ns_fx a, ns_fx b, ns_fx t);

void ns_init(NS_World *world);

ns_id ns_define_type(NS_World *world,
                     const char *name,
                     int scope,
                     int kind,
                     ns_fx initial_value,
                     ns_fx min_value,
                     ns_fx max_value,
                     int overflow,
                     int flags);

int ns_set_type_rates(NS_World *world,
                      ns_id type_id,
                      ns_fx regen_per_tick,
                      ns_fx drain_per_tick);

int ns_define_pack(NS_World *world, const NS_PackItem *items, int count);

ns_id ns_find_type(const NS_World *world, const char *name);
const char *ns_type_name(const NS_World *world, ns_id type_id);

ns_id ns_attach(NS_World *world, ns_owner owner, const char *type_name);
ns_id ns_attach_type(NS_World *world, ns_owner owner, ns_id type_id);
int ns_attach_pack(NS_World *world, ns_owner owner, const char * const *type_names, int count);
int ns_attach_type_pack(NS_World *world, ns_owner owner, const ns_id *type_ids, int count);
ns_id ns_find_value(const NS_World *world, ns_owner owner, const char *type_name);
ns_id ns_find_value_by_type(const NS_World *world, ns_owner owner, ns_id type_id);

int ns_get_by_id(const NS_World *world, ns_id value_id, ns_fx *out_value);
int ns_get_base_by_id(const NS_World *world, ns_id value_id, ns_fx *out_value);
ns_fx ns_get_or(const NS_World *world, ns_owner owner, const char *type_name, ns_fx fallback_value);

int ns_set_by_id(NS_World *world, ns_id value_id, ns_fx new_value);
int ns_set_base_by_id(NS_World *world, ns_id value_id, ns_fx new_base_value);
int ns_add_by_id(NS_World *world, ns_id value_id, ns_fx amount);
int ns_sub_by_id(NS_World *world, ns_id value_id, ns_fx amount);
int ns_reset_by_id(NS_World *world, ns_id value_id);

int ns_set(NS_World *world, ns_owner owner, const char *type_name, ns_fx new_value);
int ns_add(NS_World *world, ns_owner owner, const char *type_name, ns_fx amount);
int ns_sub(NS_World *world, ns_owner owner, const char *type_name, ns_fx amount);
int ns_reset(NS_World *world, ns_owner owner, const char *type_name);

int ns_set_bounds_by_id(NS_World *world, ns_id value_id, ns_fx min_value, ns_fx max_value);
int ns_set_rates_by_id(NS_World *world, ns_id value_id, ns_fx regen_per_tick, ns_fx drain_per_tick);
int ns_set_overflow_by_id(NS_World *world, ns_id value_id, int overflow);
int ns_set_spill_target_by_id(NS_World *world, ns_id value_id, ns_id target_value_id);
int ns_set_tag_by_id(NS_World *world, ns_id value_id, int tag);
int ns_get_tag_by_id(const NS_World *world, ns_id value_id, int *out_tag);
int ns_set_flags_by_id(NS_World *world, ns_id value_id, int flags);
int ns_add_flags_by_id(NS_World *world, ns_id value_id, int flags);
int ns_clear_flags_by_id(NS_World *world, ns_id value_id, int flags);
int ns_get_flags_by_id(const NS_World *world, ns_id value_id, int *out_flags);
int ns_was_changed_by_id(const NS_World *world, ns_id value_id);
int ns_is_dirty_by_id(const NS_World *world, ns_id value_id);
int ns_clear_changed_by_id(NS_World *world, ns_id value_id);
int ns_clear_all_changed(NS_World *world);

int ns_compare_by_id(const NS_World *world, ns_id value_id, int cmp, ns_fx rhs);
int ns_compare(const NS_World *world, ns_owner owner, const char *type_name, int cmp, ns_fx rhs);

int ns_is_empty_by_id(const NS_World *world, ns_id value_id);
int ns_is_full_by_id(const NS_World *world, ns_id value_id);
int ns_is_empty(const NS_World *world, ns_owner owner, const char *type_name);
int ns_is_full(const NS_World *world, ns_owner owner, const char *type_name);

int ns_percent_by_id(const NS_World *world, ns_id value_id, ns_fx *out_percent);
int ns_percent(const NS_World *world, ns_owner owner, const char *type_name, ns_fx *out_percent);

int ns_transfer_by_id(NS_World *world,
                      ns_id from_value_id,
                      ns_id to_value_id,
                      ns_fx requested_amount,
                      ns_fx *out_moved_amount);

int ns_sub_spill_by_id(NS_World *world,
                       ns_id primary_value_id,
                       ns_id secondary_value_id,
                       ns_fx amount,
                       ns_fx *out_spill_amount);

int ns_tick(NS_World *world);

ns_id ns_add_threshold(NS_World *world,
                       ns_id value_id,
                       ns_fx threshold,
                       int edge,
                       int once,
                       int user_code);

int ns_clear_thresholds_for_value(NS_World *world, ns_id value_id);

ns_id ns_define_template(NS_World *world, const char *name, int flags);
ns_id ns_find_template(const NS_World *world, const char *name);
int ns_template_add_type(NS_World *world, ns_id template_id, ns_id type_id);
int ns_template_add(NS_World *world, const char *template_name, const char *type_name);
int ns_attach_template(NS_World *world, ns_owner owner, ns_id template_id);
int ns_attach_template_name(NS_World *world, ns_owner owner, const char *template_name);
int ns_clear_template(NS_World *world, ns_id template_id);

ns_id ns_add_modifier_by_id(NS_World *world,
                            ns_id value_id,
                            int code,
                            int target,
                            int mode,
                            ns_fx amount,
                            int duration_ticks,
                            int priority,
                            int flags);

int ns_remove_modifier_by_code(NS_World *world, ns_id value_id, int code);
int ns_clear_modifiers_by_value(NS_World *world, ns_id value_id);
int ns_count_modifiers_by_value(const NS_World *world, ns_id value_id);
int ns_recalculate_by_id(NS_World *world, ns_id value_id);

ns_id ns_add_derived(NS_World *world,
                     ns_id output_value_id,
                     ns_id a_value_id,
                     ns_id b_value_id,
                     int mode,
                     ns_fx scale,
                     ns_fx offset,
                     int flags);
int ns_update_derived(NS_World *world, ns_id derived_id);
int ns_update_all_derived(NS_World *world);
int ns_clear_derived_for_value(NS_World *world, ns_id value_id);

void ns_query_all(NS_Query *query);
int ns_query_next(const NS_World *world, const NS_Query *query, ns_id start_after, ns_id *out_value_id);
int ns_query_count(const NS_World *world, const NS_Query *query, int *out_count);
int ns_query_add(NS_World *world, const NS_Query *query, ns_fx amount, int *out_affected);
int ns_query_sub(NS_World *world, const NS_Query *query, ns_fx amount, int *out_affected);
int ns_query_set(NS_World *world, const NS_Query *query, ns_fx value, int *out_affected);

ns_id ns_bind_value(NS_World *world, ns_id value_id, int kind, int user_code, int channel, int flags);
int ns_get_binding(const NS_World *world, ns_id binding_id, NS_Binding *out_binding);
int ns_clear_bindings_for_value(NS_World *world, ns_id value_id);
int ns_binding_count(const NS_World *world, ns_id value_id, int *out_count);

int ns_export_values(const NS_World *world, NS_SnapshotValue *out_values, int max_out, int *out_count);
int ns_import_values(NS_World *world, const NS_SnapshotValue *values, int count);
int ns_save_values(const NS_World *world, NS_WriteProc write_proc, void *user_data);
int ns_load_values(NS_World *world, NS_ReadProc read_proc, void *user_data, int count);

int ns_clear_owner(NS_World *world, ns_owner owner);
int ns_count_values(const NS_World *world, int *out_count);

void ns_set_callback(NS_World *world, NS_EventProc callback, void *user_data);
int ns_poll_event(NS_World *world, NS_Event *out_event);
int ns_dispatch_events(NS_World *world);
void ns_clear_events(NS_World *world);

#ifdef __cplusplus
}
#endif

#endif
