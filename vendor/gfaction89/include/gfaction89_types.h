#ifndef GFACTION89_TYPES_H
#define GFACTION89_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#define GFA_MAX_NAME              32
#define GFA_MAX_FACTIONS          32
#define GFA_MAX_TEAMS             32
#define GFA_MAX_ROLES             32
#define GFA_MAX_TAGS              64
#define GFA_MAX_ENTITIES          512
#define GFA_MAX_REL_RULES         512
#define GFA_MAX_TAG_RULES         256
#define GFA_MAX_ROLE_RULES        128
#define GFA_MAX_OVERRIDES         256
#define GFA_MAX_TRIGGERS          128
#define GFA_MAX_HOOKS             16
#define GFA_MAX_TOKENS            16
#define GFA_INVALID_ID            (-1)

#define GFA_TRUE                  1
#define GFA_FALSE                 0

#define GFA_FP_SHIFT              8
#define GFA_FP_ONE                (1 << GFA_FP_SHIFT)
#define GFA_FP_FROM_INT(x)        ((x) << GFA_FP_SHIFT)
#define GFA_FP_TO_INT(x)          ((x) >> GFA_FP_SHIFT)
#define GFA_FP_MUL(a,b)           (((a) * (b)) >> GFA_FP_SHIFT)
#define GFA_FP_DIV(a,b)           (((a) << GFA_FP_SHIFT) / (b))

#define GFA_TAG_WORDS             2

#define GFA_FLAG_CAN_ATTACK       0x0001
#define GFA_FLAG_CAN_ASSIST       0x0002
#define GFA_FLAG_CAN_FLEE         0x0004
#define GFA_FLAG_CAN_PROTECT      0x0008
#define GFA_FLAG_FRIENDLY_FIRE    0x0010
#define GFA_FLAG_CAN_IGNORE       0x0020
#define GFA_FLAG_CAN_FOLLOW       0x0040
#define GFA_FLAG_CAN_CONTAIN      0x0080
#define GFA_FLAG_CAN_CALL_HELP    0x0100
#define GFA_FLAG_CAN_CONVERT      0x0200
#define GFA_FLAG_SCRIPTED         0x0400
#define GFA_FLAG_VALID            0x8000

#define GFA_STIM_NONE             0
#define GFA_STIM_SIGHT            1
#define GFA_STIM_SOUND            2
#define GFA_STIM_DAMAGE           3
#define GFA_STIM_TOUCH            4
#define GFA_STIM_SCRIPT           5

#define GFA_EVENT_NONE            0
#define GFA_EVENT_DEATH           1
#define GFA_EVENT_DAMAGE          2
#define GFA_EVENT_SENSE           3
#define GFA_EVENT_TOUCH           4
#define GFA_EVENT_SCRIPT          5

#define GFA_MATCH_ANY             (-1)

#define GFA_DISP_IGNORE           0
#define GFA_DISP_NEUTRAL          1
#define GFA_DISP_ALLY             2
#define GFA_DISP_FRIENDLY         3
#define GFA_DISP_HATE             4
#define GFA_DISP_FEAR             5
#define GFA_DISP_PREY             6
#define GFA_DISP_PROTECT          7
#define GFA_DISP_RIVAL            8
#define GFA_DISP_CONTAIN          9
#define GFA_DISP_AVOID            10
#define GFA_DISP_OWNER            11
#define GFA_DISP_SCRIPTED         12
#define GFA_DISP_MAX              13

typedef int GFA_Fixed;

typedef struct GFA_TagMask {
    unsigned long word[GFA_TAG_WORDS];
} GFA_TagMask;

typedef struct GFA_NamedId {
    char name[GFA_MAX_NAME];
    int used;
} GFA_NamedId;

typedef struct GFA_Entity {
    int used;
    int entity_id;
    int faction_id;
    int team_id;
    int role_id;
    GFA_TagMask tags;
    int threat;
    int morale;
    int can_be_targeted;
    int alive;
    int user_i0;
    int user_i1;
} GFA_Entity;

typedef struct GFA_Relation {
    int disposition;
    int priority;
    int flags;
    int score_bias;
} GFA_Relation;

typedef struct GFA_TagRule {
    int used;
    int src_tag;
    int dst_tag;
    int disposition;
    int priority;
    int flags;
    int score_bias;
} GFA_TagRule;

typedef struct GFA_RoleRule {
    int used;
    int src_role;
    int dst_role;
    int disposition;
    int priority;
    int flags;
    int score_bias;
} GFA_RoleRule;

typedef struct GFA_Override {
    int used;
    int src_entity;
    int dst_entity;
    int disposition;
    int priority;
    int flags;
    int score_bias;
} GFA_Override;

typedef struct GFA_Context {
    int stimulus;
    int distance_fp;
    int visible;
    int heard;
    int recent_damage;
    int target_threat;
    int target_morale;
    int mission_bias;
    int reserved0;
    int reserved1;
} GFA_Context;

typedef struct GFA_Decision {
    int disposition;
    int priority;
    int flags;
    int score_fp;
    int can_attack;
    int can_assist;
    int can_flee_from;
    int can_protect;
    int can_follow;
    int can_call_help;
    int source_kind;
} GFA_Decision;

typedef struct GFA_Event {
    int event_type;
    int source_entity;
    int target_entity;
    int amount;
    int user_code;
} GFA_Event;

typedef struct GFA_Trigger {
    int used;
    char name[GFA_MAX_NAME];
    int event_type;
    int src_faction;
    int dst_faction;
    int src_team;
    int dst_team;
    int src_role;
    int dst_role;
    int src_tag;
    int dst_tag;
    int required_disposition;
    int set_src_faction;
    int set_dst_faction;
    int set_src_team;
    int set_dst_team;
    int set_src_role;
    int set_dst_role;
    int add_src_tag;
    int add_dst_tag;
    int clear_src_tag;
    int clear_dst_tag;
    int mark_src_alive;
    int mark_dst_alive;
    int flags_to_add_dst;
    int flags_to_add_src;
} GFA_Trigger;

typedef struct GFA_Arena {
    unsigned char *data;
    int capacity;
    int used;
} GFA_Arena;

struct GFA_World;

typedef void (*GFA_OnFactionChanged)(struct GFA_World *world, int entity_id, int old_faction, int new_faction, void *user);
typedef void (*GFA_OnDecision)(struct GFA_World *world, int src_entity, int dst_entity, const GFA_Decision *decision, void *user);
typedef void (*GFA_OnTrigger)(struct GFA_World *world, const GFA_Event *event_data, const GFA_Trigger *trigger, void *user);

typedef struct GFA_Hooks {
    GFA_OnFactionChanged on_faction_changed;
    GFA_OnDecision on_decision;
    GFA_OnTrigger on_trigger;
    void *user;
} GFA_Hooks;

typedef struct GFA_World {
    GFA_NamedId factions[GFA_MAX_FACTIONS];
    GFA_NamedId teams[GFA_MAX_TEAMS];
    GFA_NamedId roles[GFA_MAX_ROLES];
    GFA_NamedId tags[GFA_MAX_TAGS];
    GFA_Entity entities[GFA_MAX_ENTITIES];
    GFA_Relation matrix[GFA_MAX_FACTIONS][GFA_MAX_FACTIONS];
    GFA_TagRule tag_rules[GFA_MAX_TAG_RULES];
    GFA_RoleRule role_rules[GFA_MAX_ROLE_RULES];
    GFA_Override overrides[GFA_MAX_OVERRIDES];
    GFA_Trigger triggers[GFA_MAX_TRIGGERS];
    GFA_Hooks hooks[GFA_MAX_HOOKS];
    int faction_count;
    int team_count;
    int role_count;
    int tag_count;
    int entity_count;
    int default_disposition;
    int default_priority;
    int default_flags;
    int tick;
    GFA_Arena scratch;
} GFA_World;

#ifdef __cplusplus
}
#endif

#endif
