# 3D_NPC_Eyes design

## Goal

3D_NPC_Eyes is a pure perception layer for 3D NPCs. It should answer visibility/perception questions without knowing anything about the game engine.

It is intentionally not integrated with FPIL, DDSL, scripting, physics, rendering, or an entity component system.

## Main objects

```txt
tdne_sensor
  origin
  forward/right/up directions
  shape settings
  distance settings
  masks
  line-of-sight flag

 tdne_target
  origin
  radius
  mask
  user pointer
  fixed sample offsets

 tdne_result
  visibility state
  reason
  score
  distance
  dot/projection
  ray counters
  last seen point
  first hit
```

## Sensor pipeline

```txt
for each target
  for each sample point
    test against sensor shape
    if inside shape
      call user raycast when enabled
      count clear/blocked rays
  combine sample results
```

## Why samples?

A single point is too crude for stealth and cover. A target can be represented by several sample offsets:

```txt
head
center
feet
```

That allows:

```txt
3 clear rays -> visible
1 clear ray  -> partial
0 clear rays -> occluded or rejected
```

The library stores a small fixed sample array inside `tdne_target`. The default capacity is controlled by:

```c
#define TDNE_MAX_TARGET_SAMPLES 5
```

You may override it before including/building the library.

## Cone sensor

The cone sensor uses:

```txt
origin
forward direction
maximum distance
cosine of half FOV
```

Direction values use `TDNE_DIR_SCALE`, default `1024`.

The FOV helper uses integer lookup tables. The source contains integer constants only.

## Sphere sensor

The sphere sensor only checks distance. It is useful for proximity, broad awareness, sound-like awareness, or omnidirectional creatures.

## Box sensor

The box sensor is axis-aligned around its origin. It is useful for simple AI trigger volumes and area watchers.

## Frustum sensor

The frustum sensor uses:

```txt
origin
forward/right/up directions
near distance
maximum distance
horizontal FOV
vertical FOV
```

It behaves like a simple camera pyramid and uses integer tangent lookup tables.

## Raycast callback

The library never owns or understands the world. It calls:

```c
tdne_raycast_fn
```

The engine decides what blocks vision.

```txt
3D_NPC_Eyes
  -> callback
    -> user grid / BSP / octree / physics / custom collision
```

## Rejection reasons

A target can be rejected for these reasons:

```txt
masked        target mask does not match sensor see mask
out_of_range  too far or outside near/far limits
out_of_shape  outside cone/box/frustum shape
occluded      shape test passed, but line-of-sight failed
bad_input     null pointer or invalid direction
```

## Score

Score is integer-based and currently blends:

```txt
sample visibility weight
+ distance weight
```

It is deliberately simple. AI behavior layers can interpret the score however they want.
