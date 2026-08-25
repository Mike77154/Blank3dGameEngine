# API quick reference

## World and objects

```c
void gk3d_world_init(gk3d_world *world);
int gk3d_world_add_box(...);
void gk3d_world_remove(gk3d_world *world, int id);
void gk3d_obj_set_pos(...);
```

The world contains a compile-time fixed arena controlled by `GK3D_MAX_OBJECTS`.

## GameMaker-style checks

```c
int gk3d_place_meeting(world, self, x, y, z, target);
int gk3d_place_free(world, self, x, y, z);
int gk3d_instance_place(world, self, x, y, z, target);
```

`place_meeting` returns Boolean. `instance_place` returns the first matching instance ID or `GK3D_ID_NONE`. `place_free` checks only `GK3D_FLAG_SOLID` objects.

## Construct-style checks

```c
int gk3d_overlap_at_offset(world, self, dx, dy, dz, target);
int gk3d_is_on_floor(world, self);
int gk3d_is_on_wall(world, self);
```

`is_on_floor` checks `GK3D_FLAG_SOLID | GK3D_FLAG_JUMPTHRU`; walls check solids only.

Aliases are available as `gk3d_c3_is_overlapping_at_offset`, `gk3d_c3_is_on_floor`, and `gk3d_c3_is_by_wall`.

## Verb checker

```c
gk3d_condition_result result;

gk3d_check_verb(&world, "isOnFloor", self,
    GK3D_TARGET_SOLID,
    0, 0, 0,
    0, 0, 0,
    &result);
```

For semantic contact verbs, the checker applies the engine-style target automatically: floor means solid or jump-thru; wall and ceiling mean solid.

Recognized names include:

- `place_meeting`, `placeMeeting`
- `place_free`, `placeFree`
- `instance_place`, `instancePlace`
- `is_on_floor`, `isonfloor`, `isOnFloor`
- `is_on_wall`, `isonwall`, `is_by_wall`, `isByWall`
- `is_under_ceiling`, `isUnderCeiling`
- `overlap_at_offset`, `is_overlapping_at_offset`, `isOverlappingAtOffset`

## Precise collision callback

```c
int narrowphase(const gk3d_world *world,
    int moving_id,
    const gk3d_aabb *moving_box,
    int other_id,
    void *user);
```

Return:

- `GK3D_NARROWPHASE_YES`
- `GK3D_NARROWPHASE_NO`
- `GK3D_NARROWPHASE_UNHANDLED` to accept deterministic AABB fallback

## Solver

```c
gk3d_solve_result solved;
gk3d_solve_move(&world, self, dx, dy, dz,
    GK3D_TARGET_SOLID, &solved);
gk3d_apply_solved_move(&world, self, &solved);
```
