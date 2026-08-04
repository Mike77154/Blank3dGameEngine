# API quick reference

## Fixed-point constants

```c
EAI_Fixed one;
EAI_Fixed half;
EAI_Fixed five;

one = EAI_FX_ONE;
half = EAI_FX_FROM_RAW(32768);
five = EAI_FX_FROM_INT(5);
```

## Bring-up

```c
EAI_Context ctx;
EAI_WorldOps ops;

ops.raycast_world = my_raycast_world;
ops.raycast_entity = 0;
ops.edge_cost = 0;

eai_init(&ctx, &ops, my_world_ptr);
```

## Navigation

```c
EAI_NodeId node;
EAI_Fixed cost;

cost = EAI_FX_FROM_INT(5);
node = eai_nav_add_node(&ctx, &pos, zone_id, flags);
eai_nav_add_bidirectional_edge(&ctx, a, b, cost, 0);
eai_nav_find_path(&ctx, entity_id, from_node, to_node, &path);
```

## Entities

```c
EAI_EntityId id;

id = eai_entity_create(&ctx);
eai_entity_set_position(&ctx, id, &pos);
eai_entity_set_forward(&ctx, id, &forward);
eai_entity_set_team(&ctx, id, team);
eai_entity_set_view(&ctx, id, EAI_FX_FROM_INT(20), EAI_FX_FROM_RAW(46341));
```

## Perception / targeting

```c
eai_update(&ctx, EAI_FX_FROM_RAW(6554)); /* about 0.1 seconds */
best = eai_targeting_select_best(&ctx, self_id);
visible = eai_perception_is_visible(&ctx, self_id, other_id);
```

## Tactical

```c
eai_tactical_find_cover(&ctx, self_id, threat_id, &cover_node);
eai_tactical_find_flank(&ctx, self_id, target_id, &flank_node);
```

## Actions

```c
eai_actions_move_to(&ctx, self_id, &goal);
eai_actions_follow_path(&ctx, self_id, &path);
eai_actions_fire_at(&ctx, self_id, target_id);
```

## Events

```c
EAI_Event ev;
while (eai_events_pop(&ctx, &ev))
{
    /* feed your FSM */
}
```
