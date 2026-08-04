#ifndef EAI_CONTEXT_H
#define EAI_CONTEXT_H

#include "eai_types.h"
#include "eai_world.h"
#include "eai_events.h"
#include "eai_actions.h"

#define EAI_ENTITY_FLAG_CAN_SEE    0x01u
#define EAI_ENTITY_FLAG_CAN_HEAR   0x02u
#define EAI_ENTITY_FLAG_LOCK_ZONE  0x04u
#define EAI_ENTITY_FLAG_DISABLED   0x08u

#define EAI_NODE_FLAG_DISABLED     0x01u
#define EAI_NODE_FLAG_COVER        0x02u
#define EAI_NODE_FLAG_PATROL       0x04u

#define EAI_EDGE_FLAG_DISABLED     0x01u
#define EAI_EDGE_FLAG_ONE_WAY      0x02u

#define EAI_ZONE_FLAG_DISABLED     0x01u
#define EAI_ZONE_FLAG_LOCKED       0x02u

typedef struct EAI_EntityMemory
{
    EAI_EntityId target_id;
    EAI_EntityId last_seen_entity;
    EAI_EntityId last_heard_source;
    EAI_Vec3 last_seen_pos;
    EAI_Vec3 last_heard_pos;
    EAI_Fixed alertness;
    EAI_Fixed suspicion;
    EAI_Fixed target_visible_time;
    EAI_Fixed target_lost_time;
    EAI_NodeId cover_node;
    EAI_ZoneId home_zone;
} EAI_EntityMemory;

typedef struct EAI_Entity
{
    EAI_U8 used;
    EAI_U8 team;
    EAI_U8 flags;
    EAI_U8 reserved;
    EAI_Vec3 pos;
    EAI_Vec3 forward;
    EAI_NodeId current_node;
    EAI_ZoneId zone_lock_id;
    EAI_Fixed view_range;
    EAI_Fixed view_cos_half_fov;
    EAI_Fixed hearing_range;
    EAI_EntityMemory memory;
} EAI_Entity;

typedef struct EAI_Node
{
    EAI_U8 used;
    EAI_U8 flags;
    EAI_ZoneId zone_id;
    EAI_EdgeId first_edge;
    EAI_Vec3 pos;
} EAI_Node;

typedef struct EAI_Edge
{
    EAI_U8 used;
    EAI_U8 flags;
    EAI_NodeId from;
    EAI_NodeId to;
    EAI_EdgeId next_from;
    EAI_Fixed cost;
} EAI_Edge;

typedef struct EAI_Zone
{
    EAI_U8 used;
    EAI_U8 flags;
} EAI_Zone;

typedef struct EAI_Stimulus
{
    EAI_U8 active;
    EAI_U8 kind;
    EAI_EntityId source_id;
    EAI_Vec3 pos;
    EAI_Fixed radius;
    EAI_Fixed strength;
    EAI_Fixed age;
    EAI_U32 heard_mask[EAI_SOUND_MASK_WORDS];
} EAI_Stimulus;

typedef struct EAI_AStarRecord
{
    EAI_U8 opened;
    EAI_U8 closed;
    EAI_NodeId parent;
    EAI_Fixed g;
    EAI_Fixed f;
} EAI_AStarRecord;

struct EAI_Context
{
    EAI_WorldOps world_ops;
    void* world_user;

    EAI_Entity entities[EAI_MAX_ENTITIES];
    EAI_Node   nodes[EAI_MAX_NAV_NODES];
    EAI_Edge   edges[EAI_MAX_NAV_EDGES];
    EAI_Zone   zones[EAI_MAX_ZONES];

    EAI_U8 hostility[EAI_MAX_TEAMS][EAI_MAX_TEAMS];
    EAI_U8 visibility[EAI_MAX_ENTITIES][EAI_MAX_ENTITIES];

    EAI_Event event_queue[EAI_MAX_EVENTS];
    EAI_U16 event_head;
    EAI_U16 event_tail;

    EAI_Stimulus stimuli[EAI_MAX_STIMULI];
    EAI_ActionIntent intents[EAI_MAX_ENTITIES];

    EAI_AStarRecord astar[EAI_MAX_NAV_NODES];
    EAI_NodeId open_nodes[EAI_MAX_SEARCH_OPEN];
    EAI_U16 open_count;

    EAI_U16 entity_count;
    EAI_U16 node_count;
    EAI_U16 edge_count;
    EAI_U16 zone_count;
};

#endif
