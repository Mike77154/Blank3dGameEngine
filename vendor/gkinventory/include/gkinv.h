#ifndef GKINV_H
#define GKINV_H

#include "gkinv_fixed.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Result codes. Negative values are errors. */
enum {
    GKINV_OK = 0,
    GKINV_ERR_NULL = -1,
    GKINV_ERR_BAD_ITEM = -2,
    GKINV_ERR_BAD_QUANTITY = -3,
    GKINV_ERR_NOT_FOUND = -4,
    GKINV_ERR_FULL = -5,
    GKINV_ERR_NO_ROOM = -6,
    GKINV_ERR_BAD_INDEX = -7,
    GKINV_ERR_STACK_LIMIT = -8,
    GKINV_ERR_CATEGORY = -9,
    GKINV_ERR_FLAGS = -10,
    GKINV_ERR_WEIGHT = -11,
    GKINV_ERR_RULE = -12,
    GKINV_ERR_GRID = -13,
    GKINV_ERR_TEMP_LIMIT = -14,
    GKINV_ERR_READ_ONLY = -15,
    GKINV_ERR_NO_HOOK = -16,
    GKINV_ERR_PARSE = -17
};

/* Item flags. The core treats these as generic semantics, not genre rules. */
#define GKINV_ITEM_STACKABLE     ((gkinv_u32)(1UL << 0))
#define GKINV_ITEM_CONSUMABLE    ((gkinv_u32)(1UL << 1))
#define GKINV_ITEM_EQUIPPABLE    ((gkinv_u32)(1UL << 2))
#define GKINV_ITEM_KEY           ((gkinv_u32)(1UL << 3))
#define GKINV_ITEM_UNIQUE        ((gkinv_u32)(1UL << 4))
#define GKINV_ITEM_NO_DISCARD    ((gkinv_u32)(1UL << 5))
#define GKINV_ITEM_COMBINABLE    ((gkinv_u32)(1UL << 6))
#define GKINV_ITEM_USABLE        ((gkinv_u32)(1UL << 7))
#define GKINV_ITEM_PERSIST_STATE ((gkinv_u32)(1UL << 8))

/* Container flags. */
#define GKINV_CONT_STACKS        ((gkinv_u32)(1UL << 0))
#define GKINV_CONT_GRID          ((gkinv_u32)(1UL << 1))
#define GKINV_CONT_EQUIPMENT     ((gkinv_u32)(1UL << 2))
#define GKINV_CONT_READ_ONLY     ((gkinv_u32)(1UL << 3))
#define GKINV_CONT_ALLOW_SWAP    ((gkinv_u32)(1UL << 4))
#define GKINV_CONT_ALLOW_PARTIAL ((gkinv_u32)(1UL << 5))

/* Generic category helper bits. Projects can define their own masks. */
#define GKINV_CAT_ANY            ((gkinv_u32)0UL)
#define GKINV_CAT_CONSUMABLE     ((gkinv_u32)(1UL << 0))
#define GKINV_CAT_WEAPON         ((gkinv_u32)(1UL << 1))
#define GKINV_CAT_AMMO           ((gkinv_u32)(1UL << 2))
#define GKINV_CAT_ARMOR          ((gkinv_u32)(1UL << 3))
#define GKINV_CAT_KEY            ((gkinv_u32)(1UL << 4))
#define GKINV_CAT_PUZZLE         ((gkinv_u32)(1UL << 5))
#define GKINV_CAT_RESOURCE       ((gkinv_u32)(1UL << 6))
#define GKINV_CAT_DOCUMENT       ((gkinv_u32)(1UL << 7))

#define GKINV_RECIPE_ORDERED     ((gkinv_u32)(1UL << 0))
#define GKINV_RECIPE_KEEP_A_STATE ((gkinv_u32)(1UL << 1))
#define GKINV_RECIPE_KEEP_B_STATE ((gkinv_u32)(1UL << 2))

struct gkinv_Container;
struct gkinv_Event;

typedef struct gkinv_ItemDef {
    gkinv_u16 id;
    const char *name;
    gkinv_u16 max_stack;
    gkinv_u16 grid_w;
    gkinv_u16 grid_h;
    gkinv_u32 flags;
    gkinv_u32 category_mask;
    gkinv_fx weight;
    gkinv_i32 user0;
    gkinv_i32 user1;
} gkinv_ItemDef;

typedef struct gkinv_ItemDB {
    const gkinv_ItemDef *items;
    gkinv_u16 count;
} gkinv_ItemDB;

typedef struct gkinv_ItemStack {
    gkinv_u16 item_id;
    gkinv_u16 quantity;
    gkinv_u32 flags;
    gkinv_i32 state0;
    gkinv_i32 state1;
    gkinv_i32 state2;
    gkinv_i32 state3;
} gkinv_ItemStack;

typedef struct gkinv_Slot {
    gkinv_u16 item_id;
    gkinv_u16 quantity;
    gkinv_u16 x;
    gkinv_u16 y;
    gkinv_u32 flags;
    gkinv_i32 state0;
    gkinv_i32 state1;
    gkinv_i32 state2;
    gkinv_i32 state3;
} gkinv_Slot;

typedef struct gkinv_SlotRule {
    gkinv_u16 slot_index;
    gkinv_u32 accept_categories;
    gkinv_u32 require_flags;
    gkinv_u32 reject_flags;
    gkinv_u16 max_stack;
} gkinv_SlotRule;

typedef struct gkinv_Recipe {
    gkinv_u16 item_a;
    gkinv_u16 qty_a;
    gkinv_u16 item_b;
    gkinv_u16 qty_b;
    gkinv_u16 result_item;
    gkinv_u16 result_qty;
    gkinv_u32 flags;
} gkinv_Recipe;

typedef struct gkinv_Event {
    int type;
    int result;
    struct gkinv_Container *src;
    struct gkinv_Container *dst;
    gkinv_u16 item_id;
    gkinv_u16 quantity;
    gkinv_u16 src_index;
    gkinv_u16 dst_index;
} gkinv_Event;

enum {
    GKINV_EVENT_ADDED = 1,
    GKINV_EVENT_REMOVED = 2,
    GKINV_EVENT_MOVED = 3,
    GKINV_EVENT_TRANSFERRED = 4,
    GKINV_EVENT_USED = 5,
    GKINV_EVENT_COMBINED = 6,
    GKINV_EVENT_CLEARED = 7,
    GKINV_EVENT_LOADED = 8
};

typedef int (*gkinv_CanAddFn)(struct gkinv_Container *container,
                              const gkinv_ItemDB *db,
                              const gkinv_ItemDef *def,
                              const gkinv_ItemStack *stack,
                              gkinv_u16 dst_index,
                              void *user);

typedef int (*gkinv_CanRemoveFn)(struct gkinv_Container *container,
                                 const gkinv_ItemDB *db,
                                 const gkinv_ItemDef *def,
                                 gkinv_u16 src_index,
                                 gkinv_u16 quantity,
                                 void *user);

typedef int (*gkinv_OnUseFn)(struct gkinv_Container *container,
                             const gkinv_ItemDB *db,
                             gkinv_u16 slot_index,
                             void *actor,
                             void *target,
                             void *user);

typedef void (*gkinv_EventFn)(const gkinv_Event *event, void *user);

typedef struct gkinv_Hooks {
    gkinv_CanAddFn can_add;
    gkinv_CanRemoveFn can_remove;
    gkinv_OnUseFn on_use;
    gkinv_EventFn on_event;
} gkinv_Hooks;

typedef struct gkinv_Container {
    gkinv_Slot *slots;
    gkinv_u16 slot_capacity;
    gkinv_u16 count;
    gkinv_u16 grid_w;
    gkinv_u16 grid_h;
    gkinv_u32 flags;
    gkinv_u32 allowed_categories;
    gkinv_fx max_weight;
    const gkinv_SlotRule *slot_rules;
    gkinv_u16 slot_rule_count;
    const gkinv_Hooks *hooks;
    void *hook_user;
    void *user;
} gkinv_Container;

typedef int (*gkinv_TextWriter)(void *user, const char *data, gkinv_u16 size);

#define GKINV_DECLARE_SLOTS(name, capacity) gkinv_Slot name[(capacity)]

void gkinv_slot_clear(gkinv_Slot *slot);
void gkinv_stack_clear(gkinv_ItemStack *stack);
int gkinv_stack_make(gkinv_ItemStack *stack, gkinv_u16 item_id, gkinv_u16 quantity);

void gkinv_container_init(gkinv_Container *container,
                          gkinv_Slot *slots,
                          gkinv_u16 slot_capacity,
                          gkinv_u32 flags);
void gkinv_container_clear(gkinv_Container *container);
void gkinv_container_set_grid(gkinv_Container *container, gkinv_u16 grid_w, gkinv_u16 grid_h);
void gkinv_container_set_limits(gkinv_Container *container,
                                gkinv_u32 allowed_categories,
                                gkinv_fx max_weight);
void gkinv_container_set_slot_rules(gkinv_Container *container,
                                    const gkinv_SlotRule *rules,
                                    gkinv_u16 rule_count);
void gkinv_container_set_hooks(gkinv_Container *container,
                               const gkinv_Hooks *hooks,
                               void *hook_user);

const gkinv_ItemDef *gkinv_db_find(const gkinv_ItemDB *db, gkinv_u16 item_id);
gkinv_u16 gkinv_item_max_stack(const gkinv_ItemDef *def);
gkinv_u16 gkinv_item_grid_w(const gkinv_ItemDef *def);
gkinv_u16 gkinv_item_grid_h(const gkinv_ItemDef *def);

int gkinv_validate_container(const gkinv_ItemDB *db, const gkinv_Container *container);
gkinv_fx gkinv_total_weight(const gkinv_ItemDB *db, const gkinv_Container *container);

int gkinv_has_item(const gkinv_Container *container, gkinv_u16 item_id, gkinv_u16 quantity);
gkinv_u16 gkinv_count_item(const gkinv_Container *container, gkinv_u16 item_id);
int gkinv_find_item(const gkinv_Container *container, gkinv_u16 item_id, gkinv_u16 *out_index);

int gkinv_can_add_stack(const gkinv_ItemDB *db, const gkinv_Container *container, const gkinv_ItemStack *stack);
int gkinv_add_item(const gkinv_ItemDB *db, gkinv_Container *container, gkinv_u16 item_id, gkinv_u16 quantity, gkinv_u16 *out_index);
int gkinv_add_stack(const gkinv_ItemDB *db, gkinv_Container *container, const gkinv_ItemStack *stack, gkinv_u16 *out_index);
int gkinv_insert_stack_at(const gkinv_ItemDB *db, gkinv_Container *container, const gkinv_ItemStack *stack, gkinv_u16 index);
int gkinv_insert_stack_grid_at(const gkinv_ItemDB *db, gkinv_Container *container, const gkinv_ItemStack *stack, gkinv_u16 index, gkinv_u16 x, gkinv_u16 y);

int gkinv_remove_item(const gkinv_ItemDB *db, gkinv_Container *container, gkinv_u16 item_id, gkinv_u16 quantity);
int gkinv_remove_from_slot(const gkinv_ItemDB *db, gkinv_Container *container, gkinv_u16 index, gkinv_u16 quantity);

int gkinv_move_slot(const gkinv_ItemDB *db, gkinv_Container *container, gkinv_u16 from_index, gkinv_u16 to_index);
int gkinv_split_slot(const gkinv_ItemDB *db, gkinv_Container *container, gkinv_u16 from_index, gkinv_u16 to_index, gkinv_u16 quantity);
int gkinv_move_grid_slot(const gkinv_ItemDB *db, gkinv_Container *container, gkinv_u16 index, gkinv_u16 x, gkinv_u16 y);

int gkinv_transfer_item(const gkinv_ItemDB *db, gkinv_Container *src, gkinv_Container *dst, gkinv_u16 item_id, gkinv_u16 quantity);
int gkinv_transfer_slot(const gkinv_ItemDB *db, gkinv_Container *src, gkinv_Container *dst, gkinv_u16 src_index, gkinv_u16 quantity);

int gkinv_use_slot(const gkinv_ItemDB *db, gkinv_Container *container, gkinv_u16 index, void *actor, void *target);
int gkinv_combine_slots(const gkinv_ItemDB *db, gkinv_Container *container, gkinv_u16 index_a, gkinv_u16 index_b, const gkinv_Recipe *recipes, gkinv_u16 recipe_count);

#if GKINV_ENABLE_TEXT_IO
int gkinv_save_text(const gkinv_Container *container, gkinv_TextWriter writer, void *user);
int gkinv_load_text(const gkinv_ItemDB *db, gkinv_Container *container, const char *text);
#endif

const char *gkinv_result_name(int result);

#ifdef __cplusplus
}
#endif

#endif
