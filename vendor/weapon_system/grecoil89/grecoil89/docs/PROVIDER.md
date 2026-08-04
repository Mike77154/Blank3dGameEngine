# External transform provider

`grecoil89_provider` lets a separate camera, animation, entity, socket or
transform library receive recoil `move` and `rotate` without depending on that
library's types.

It remains C89, fixed-point and caller-owned. It allocates no memory.

## Important semantic rule

Provider values are the **current absolute additive recoil layer**.

The external library must replace the previous recoil layer every tick:

```txt
final_rotation = base_rotation + provider_rotation
final_position = base_position + provider_move
```

Do not repeatedly add each provider value to the already modified transform,
or recoil will accumulate forever.

## Push mode

The external library gives `grecoil89` two callbacks:

```c
static void my_rotate(void *user, grec_u16 target,
                      const GRecAngles *rotate);
static void my_move(void *user, grec_u16 target,
                    const GRecVec3 *move);
```

Bind them once:

```c
GRecProvider provider;

grec_provider_init(&provider, external_lib, my_rotate, my_move);
grec_provider_set_mode(&provider, GREC_PROVIDER_MODE_PUSH);
```

After firing and after each fixed update:

```c
grec_fire(&state, profile, &ctx);
grec_provider_after_fire(&provider, &state, profile, &ctx);

grec_update(&state, profile);
grec_provider_after_update(&provider, &state, profile, &ctx);
```

Targets sent to `set_rotate`:

```txt
GREC_PROVIDER_TARGET_AIM
GREC_PROVIDER_TARGET_CAMERA
GREC_PROVIDER_TARGET_WEAPON
```

`set_move` currently receives `GREC_PROVIDER_TARGET_WEAPON` because recoil has
weapon translation only.

## Pull mode

A separate library can also query the provider when it is ready:

```c
GRecAngles rotation;
GRecVec3 move;

grec_provider_set_mode(&provider, GREC_PROVIDER_MODE_PULL);
grec_provider_after_update(&provider, &state, profile, &ctx);

if (grec_provider_get_rotate(&provider,
                             GREC_PROVIDER_TARGET_WEAPON,
                             &rotation)) {
    /* external transform library applies rotation */
}

if (grec_provider_get_move(&provider,
                           GREC_PROVIDER_TARGET_WEAPON,
                           &move)) {
    /* external transform library applies translation */
}
```

## Mask

Channels can be selected independently:

```c
grec_provider_set_mask(&provider,
    GREC_PROVIDER_CAMERA_ROTATE |
    GREC_PROVIDER_WEAPON_ROTATE |
    GREC_PROVIDER_WEAPON_MOVE);
```

## Detach/reset

Before removing a weapon or provider, send a neutral layer:

```c
grec_provider_clear(&provider);
```
