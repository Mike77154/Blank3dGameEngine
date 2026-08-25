# Engine Bridge Protocol

## Regla de oro

`scene3d89` es autoridad de escena, no autoridad de render/física/IA.

## Flujo recomendado por frame

```text
1. Scripts/engine aplican cambios de escena.
2. scene3d89 actualiza transforms.
3. Renderer pide renderables por layer.
4. Physics pide collidables por layer.
5. AI/scripting hacen queries por groups/tags/bounds.
6. Engine decide streaming/load/unload externo.
```

## Renderer bridge

```c
sm3d_feed_renderables(&ctx, scene_id, layer_mask, my_render_feed, renderer_user);
```

Recibe:

- handle,
- resource_id,
- layer_mask,
- world transform,
- world bounds.

## Physics bridge

```c
sm3d_feed_collidables(&ctx, scene_id, layer_mask, my_physics_feed, physics_user);
```

Recibe:

- handle,
- collider/resource id,
- world transform,
- world bounds.

## Lifecycle events

Puedes conectar `SM3D_Bridge.event_fn` para eventos:

- scene loaded/unloaded,
- node enter/exit,
- node moved,
- visibility changed,
- reparented,
- destroyed.

## Manifest de integración sugerido

```text
phase: phase1
module: scene3d89
role: pure_scene_authority
provides: scene_graph, additive_scenes, layers, groups, fixed_transform, aabb_queries, bridge_feeds
requires: fixed-point C89 compiler
forbidden: malloc, realloc, free, heap, float, double
```
