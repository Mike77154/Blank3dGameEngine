# Cameranaku89 receive-provider transforms

## Purpose

`CNK_TRANSFORM_MODE_RECEIVE_PROVIDER` lets a host engine or external library provide Cameranaku89's 3D transform operations without replacing the camera solver.

The provider owns no memory and Cameranaku89 performs no allocation. The camera copies the provider struct, while `provider.user` remains an opaque pointer owned by the host.

## Callback contracts

```c
typedef int (*cnk_transform_move_fn)(
    void *user,
    cnk_vec3 position,
    cnk_vec3 delta,
    cnk_vec3 *out_position
);

typedef int (*cnk_transform_scale_fn)(
    void *user,
    cnk_vec3 value,
    cnk_vec3 scale,
    cnk_vec3 *out_value
);

typedef int (*cnk_transform_rotate_fn)(
    void *user,
    cnk_vec3 value,
    cnk_vec3 euler_deg,
    cnk_vec3 *out_value
);
```

Contracts:

```text
move   -> out_position = position + delta
scale  -> component-wise scale
rotate -> rotate value by yaw/pitch/roll
```

Euler mapping:

```text
euler_deg.x = yaw
euler_deg.y = pitch
euler_deg.z = roll
```

Return non-zero when the callback produced a valid output. Return zero to request the internal fallback for that operation.

## Partial providers

A provider may supply only one or two operations. Missing callbacks fall back independently:

```text
external move + internal scale + internal rotate
external rotate + internal move + internal scale
external move + external scale + internal rotate
```

This makes incremental engine integration possible.

## Camera binding

```c
cnk_transform_provider provider;
cnk_camera camera;

cnk_transform_provider_clear(&provider);
cnk_transform_provider_set(
    &provider,
    my_math_context,
    my_move,
    my_scale,
    my_rotate);

cnk_camera_reset(&camera);
cnk_camera_set_transform_provider(&camera, &provider);
```

Disable and return to built-in math:

```c
cnk_camera_clear_transform_provider(&camera);
```

Temporarily switch the provider itself to internal mode:

```c
cnk_transform_provider_set_mode(&provider, CNK_TRANSFORM_MODE_INTERNAL);
cnk_camera_set_transform_provider(&camera, &provider);
```

## Engine adapter binding

```c
cnk_solver_bind bind;

bind.camera = &camera;
bind.target_id = player_entity_id;
bind.enabled = 1;

cnk_adapter_receive_transform_provider(&bind, &provider);
```

The ordinary adapter update remains unchanged:

```c
cnk_adapter_update_bind(&bind, &engine_adapter, dt_ticks);
```

## Virtual cameras and manager

One virtual camera:

```c
cnk_virtual_camera_set_transform_provider(vcam, &provider);
```

All current and future virtual cameras allocated by a manager:

```c
cnk_camera_manager_set_transform_provider_all(&manager, &provider);
```

## Utility TRS

The provider can also be used outside a camera:

```c
result = cnk_transform_apply_trs(
    &provider,
    point,
    scale,
    euler_deg,
    translation);
```

Order:

```text
scale -> rotate -> move
```

## Fixed-point boundary

Provider callbacks receive `cnk_fx` values. An engine with a different native representation may convert inside the callback and convert the result back before returning.

Cameranaku89 itself remains:

```text
C89
fixed-point
no malloc/realloc/free
no heap ownership
no float/double
```

## ABI note

Version 3.4.0 adds provider fields to `cnk_camera` and `cnk_camera_manager`. Recompile every translation unit or binary module that embeds or passes those structs by value.
