#ifndef PBB_ITEM_TYPES_H
#define PBB_ITEM_TYPES_H

#include "pbb_item_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PBB_ACTOR_CLASS_PLAYER  0x00000001UL
#define PBB_ACTOR_CLASS_NPC     0x00000002UL
#define PBB_ACTOR_CLASS_ENEMY   0x00000004UL
#define PBB_ACTOR_CLASS_OBJECT  0x00000008UL
#define PBB_ACTOR_CLASS_ANY     0xFFFFFFFFUL

#define PBB_ITEM_CATEGORY_PICKUP    0x00000001UL
#define PBB_ITEM_CATEGORY_USABLE    0x00000002UL
#define PBB_ITEM_CATEGORY_KEY       0x00000004UL
#define PBB_ITEM_CATEGORY_LOOT      0x00000008UL
#define PBB_ITEM_CATEGORY_SWITCH    0x00000010UL
#define PBB_ITEM_CATEGORY_HAZARD    0x00000020UL
#define PBB_ITEM_CATEGORY_QUEST     0x00000040UL
#define PBB_ITEM_CATEGORY_MISC      0x00000080UL
#define PBB_ITEM_CATEGORY_ANY       0xFFFFFFFFUL

#define PBB_ITEMF_ACTIVE             0x00000001UL
#define PBB_ITEMF_VISIBLE            0x00000002UL
#define PBB_ITEMF_TOUCHABLE          0x00000004UL
#define PBB_ITEMF_INTERACTABLE       0x00000008UL
#define PBB_ITEMF_DROPPABLE          0x00000010UL
#define PBB_ITEMF_LOCKED             0x00000020UL
#define PBB_ITEMF_PERSISTENT         0x00000040UL
#define PBB_ITEMF_SCRIPTED           0x00000080UL
#define PBB_ITEMF_RECYCLE_ON_CONSUME 0x00000100UL

enum {
    PBB_ITEM_STATE_ANY = -1,
    PBB_ITEM_STATE_NONE = 0,
    PBB_ITEM_STATE_WORLD = 1,
    PBB_ITEM_STATE_DROPPED = 2,
    PBB_ITEM_STATE_HIDDEN = 3,
    PBB_ITEM_STATE_CONSUMED = 4,
    PBB_ITEM_STATE_DISABLED = 5
};

enum {
    PBB_ITEM_HOOK_SPAWN = 0,
    PBB_ITEM_HOOK_TOUCH = 1,
    PBB_ITEM_HOOK_INTERACT = 2,
    PBB_ITEM_HOOK_DROP = 3,
    PBB_ITEM_HOOK_COUNT = 4
};

enum {
    PBB_RULE_NONE = 0,
    PBB_RULE_ALWAYS = 1,
    PBB_RULE_ACTOR_CLASS_ANY = 2,
    PBB_RULE_ACTOR_CLASS_NONE = 3,
    PBB_RULE_ACTOR_TEAM_ANY = 4,
    PBB_RULE_ACTOR_TEAM_NONE = 5,
    PBB_RULE_ACTOR_FLAG_SET = 6,
    PBB_RULE_ACTOR_FLAG_CLEAR = 7,
    PBB_RULE_WORLD_FLAG_SET = 8,
    PBB_RULE_WORLD_FLAG_CLEAR = 9,
    PBB_RULE_ITEM_FLAG_SET = 10,
    PBB_RULE_ITEM_FLAG_CLEAR = 11,
    PBB_RULE_ACTOR_VAR_EQ = 12,
    PBB_RULE_ACTOR_VAR_NE = 13,
    PBB_RULE_ACTOR_VAR_GTE = 14,
    PBB_RULE_ACTOR_VAR_LTE = 15,
    PBB_RULE_WORLD_VAR_EQ = 16,
    PBB_RULE_WORLD_VAR_NE = 17,
    PBB_RULE_WORLD_VAR_GTE = 18,
    PBB_RULE_WORLD_VAR_LTE = 19,
    PBB_RULE_ITEM_CUSTOM_EQ = 20,
    PBB_RULE_ITEM_CUSTOM_NE = 21,
    PBB_RULE_ITEM_CUSTOM_GTE = 22,
    PBB_RULE_ITEM_CUSTOM_LTE = 23,
    PBB_RULE_ITEM_STATE_EQ = 24,
    PBB_RULE_ITEM_STATE_NE = 25,
    PBB_RULE_ITEM_AMOUNT_GTE = 26,
    PBB_RULE_ITEM_AMOUNT_LTE = 27,
    PBB_RULE_TOUCH_COUNT_LT = 28,
    PBB_RULE_INTERACT_COUNT_LT = 29,
    PBB_RULE_OWNER_IS_ACTOR = 30,
    PBB_RULE_OWNER_IS_NOT_ACTOR = 31,
    PBB_RULE_ACTOR_VAR_GTE_ITEM_AMOUNT = 32,
    PBB_RULE_CALL_CUSTOM = 100
};

enum {
    PBB_EFFECT_NONE = 0,
    PBB_EFFECT_ADD_ACTOR_VAR = 1,
    PBB_EFFECT_SET_ACTOR_VAR = 2,
    PBB_EFFECT_CLAMP_ACTOR_VAR = 3,
    PBB_EFFECT_ADD_WORLD_VAR = 4,
    PBB_EFFECT_SET_WORLD_VAR = 5,
    PBB_EFFECT_CLAMP_WORLD_VAR = 6,
    PBB_EFFECT_SET_ACTOR_FLAG = 7,
    PBB_EFFECT_CLEAR_ACTOR_FLAG = 8,
    PBB_EFFECT_TOGGLE_ACTOR_FLAG = 9,
    PBB_EFFECT_SET_WORLD_FLAG = 10,
    PBB_EFFECT_CLEAR_WORLD_FLAG = 11,
    PBB_EFFECT_TOGGLE_WORLD_FLAG = 12,
    PBB_EFFECT_SET_ITEM_FLAG = 13,
    PBB_EFFECT_CLEAR_ITEM_FLAG = 14,
    PBB_EFFECT_TOGGLE_ITEM_FLAG = 15,
    PBB_EFFECT_SET_ITEM_STATE = 16,
    PBB_EFFECT_HIDE_ITEM = 17,
    PBB_EFFECT_SHOW_ITEM = 18,
    PBB_EFFECT_CONSUME_ITEM = 19,
    PBB_EFFECT_DESTROY_ITEM = 20,
    PBB_EFFECT_SET_ITEM_AMOUNT = 21,
    PBB_EFFECT_ADD_ITEM_AMOUNT = 22,
    PBB_EFFECT_SET_ITEM_CUSTOM = 23,
    PBB_EFFECT_ADD_ITEM_CUSTOM = 24,
    PBB_EFFECT_ADD_ACTOR_VAR_FROM_ITEM_AMOUNT = 25,
    PBB_EFFECT_ADD_WORLD_VAR_FROM_ITEM_AMOUNT = 26,
    PBB_EFFECT_SPAWN_ITEM_AT_ITEM = 27,
    PBB_EFFECT_SPAWN_ITEM_AT_ACTOR = 28,
    PBB_EFFECT_SET_ITEM_OWNER_ACTOR = 29,
    PBB_EFFECT_CLEAR_ITEM_OWNER = 30,
    PBB_EFFECT_MOVE_ITEM_TO_ACTOR = 31,
    PBB_EFFECT_EMIT_EVENT = 32,
    PBB_EFFECT_CALL_CUSTOM = 100
};

enum {
    PBB_EVENT_NONE = 0,
    PBB_EVENT_ITEM_SPAWNED = 1,
    PBB_EVENT_ITEM_TOUCHED = 2,
    PBB_EVENT_ITEM_INTERACTED = 3,
    PBB_EVENT_ITEM_DROPPED = 4,
    PBB_EVENT_ITEM_RULE_BLOCKED = 5,
    PBB_EVENT_ITEM_EFFECT = 6,
    PBB_EVENT_ITEM_CONSUMED = 7,
    PBB_EVENT_ITEM_DESTROYED = 8,
    PBB_EVENT_CUSTOM = 100
};

enum {
    PBB_ITEM_BLOCK_REASON_RULE = 0,
    PBB_ITEM_BLOCK_REASON_MASK = 1,
    PBB_ITEM_BLOCK_REASON_NOT_DROPPABLE = 2,
    PBB_ITEM_BLOCK_REASON_PROVIDER = 3
};

struct PBB_ItemWorld;
struct PBB_ItemRule;
struct PBB_ItemEffect;

typedef int (*PBB_ItemCustomRuleFn)(struct PBB_ItemWorld *world,
                                    int actor_id,
                                    int item_id,
                                    const struct PBB_ItemRule *rule);

typedef int (*PBB_ItemCustomEffectFn)(struct PBB_ItemWorld *world,
                                      int actor_id,
                                      int item_id,
                                      const struct PBB_ItemEffect *effect);

typedef int (*PBB_ItemActionGateFn)(struct PBB_ItemWorld *world,
                                    int actor_id,
                                    int item_id,
                                    int hook,
                                    void *user);

typedef struct PBB_ItemRule {
    int used;
    int type;
    int a;
    int b;
    int c;
    int d;
} PBB_ItemRule;

typedef struct PBB_ItemEffect {
    int used;
    int type;
    int a;
    int b;
    int c;
    int d;
    PBB_Fixed fx;
    PBB_Fixed fy;
} PBB_ItemEffect;

typedef struct PBB_ItemDef {
    int used;
    int def_id;
    char name[PBB_ITEM_NAME_SIZE];
    unsigned long default_flags;
    unsigned long default_category_mask;
    int default_amount;
    PBB_Fixed default_half_w;
    PBB_Fixed default_half_h;
    int rule_first[PBB_ITEM_HOOK_COUNT];
    int rule_count[PBB_ITEM_HOOK_COUNT];
    int effect_first[PBB_ITEM_HOOK_COUNT];
    int effect_count[PBB_ITEM_HOOK_COUNT];
} PBB_ItemDef;

typedef struct PBB_ItemActor {
    int used;
    int actor_id;
    unsigned long class_mask;
    unsigned long team_mask;
    unsigned long touch_mask;
    unsigned long interact_mask;
    PBB_Fixed x;
    PBB_Fixed y;
    PBB_Fixed half_w;
    PBB_Fixed half_h;
    int vars[PBB_ITEM_MAX_ACTOR_VARS];
    unsigned long flags[PBB_ITEM_FLAG_WORDS];
} PBB_ItemActor;

typedef struct PBB_Item {
    int used;
    int item_id;
    int def_id;
    int state;
    int amount;
    int owner_actor_id;
    unsigned long flags;
    unsigned long category_mask;
    PBB_Fixed x;
    PBB_Fixed y;
    PBB_Fixed half_w;
    PBB_Fixed half_h;
    int touch_count;
    int interact_count;
    int drop_count;
    int custom[PBB_ITEM_CUSTOM_COUNT];
} PBB_Item;

typedef struct PBB_ItemEvent {
    int used;
    int type;
    int actor_id;
    int item_id;
    int def_id;
    int a;
    int b;
    int c;
    PBB_Fixed x;
    PBB_Fixed y;
} PBB_ItemEvent;

typedef struct PBB_ItemDropTable {
    int used;
    int table_id;
    char name[PBB_ITEM_NAME_SIZE];
} PBB_ItemDropTable;

typedef struct PBB_ItemDropEntry {
    int used;
    int table_id;
    int def_id;
    int min_amount;
    int max_amount;
    int chance_per_10000;
    PBB_Fixed offset_x;
    PBB_Fixed offset_y;
} PBB_ItemDropEntry;

typedef struct PBB_ItemWorld {
    PBB_ItemActor actors[PBB_ITEM_MAX_ACTORS];
    PBB_Item items[PBB_ITEM_MAX_ITEMS];
    PBB_ItemDef defs[PBB_ITEM_MAX_DEFS];
    PBB_ItemRule rules[PBB_ITEM_MAX_RULES];
    PBB_ItemEffect effects[PBB_ITEM_MAX_EFFECTS];
    PBB_ItemEvent events[PBB_ITEM_MAX_EVENTS];
    PBB_ItemDropTable drop_tables[PBB_ITEM_MAX_DROP_TABLES];
    PBB_ItemDropEntry drop_entries[PBB_ITEM_MAX_DROP_ENTRIES];
    int world_vars[PBB_ITEM_MAX_WORLD_VARS];
    unsigned long world_flags[PBB_ITEM_FLAG_WORDS];
    int event_read;
    int event_write;
    int event_count;
    int events_lost;
    unsigned long rng_state;
    int strict_droppable;
    int recycle_consumed_items;
    PBB_ItemCustomRuleFn custom_rule;
    PBB_ItemCustomEffectFn custom_effect;
    void *user_data;
    PBB_ItemActionGateFn action_gate;
    void *action_gate_user;
} PBB_ItemWorld;

typedef struct PBB_ItemIter {
    int index;
    unsigned long required_flags;
    unsigned long rejected_flags;
    int state;
    unsigned long category_mask;
} PBB_ItemIter;

#ifdef __cplusplus
}
#endif

#endif
