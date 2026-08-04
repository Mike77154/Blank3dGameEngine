# AI bridge pattern

`gfaction89` does not move, animate, pathfind, aim, or shoot. It only answers whether an entity is a legal/interesting target.

## Generic target selection

```c
int candidates[32];
int candidate_count;
GFA_Context ctx;
GFA_Decision d;
int best;

candidate_count = my_sensor_collect_visible(self_id, candidates, 32);
ctx.stimulus = GFA_STIM_SIGHT;
ctx.distance_fp = GFA_FP_FROM_INT(8);
ctx.visible = 1;
ctx.heard = 0;
ctx.recent_damage = 0;
ctx.mission_bias = 0;

best = gfa_choose_best_target(&world, self_id, candidates, candidate_count, &ctx, &d);
if (best >= 0) {
    my_ai_set_target(self_id, best);
}
```

## Civilian fleeing

```c
d = gfa_eval(&world, civilian_id, candidate_id, &ctx);
if (d.can_flee_from) {
    my_ai_flee_from(civilian_id, candidate_id);
}
```

## Ally assistance

```c
d = gfa_eval(&world, cop_id, civilian_id, &ctx);
if (d.can_protect) {
    my_ai_protect(cop_id, civilian_id);
}
```

## Hooking runtime faction changes

```c
static void on_faction_changed(GFA_World *world, int entity_id, int old_faction, int new_faction, void *user)
{
    my_engine_refresh_badge(entity_id, new_faction);
    my_engine_rebuild_sensor_cache(entity_id);
}
```

## 2D/3D independence

Distance, visibility, hearing, and damage are provided by the host engine. This library does not know coordinates, meshes, sprites, or physics.
