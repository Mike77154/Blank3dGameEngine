# Adapter notes

This folder is intentionally light. PDC3D exposes bridge hooks instead of hard-linking to one engine.

Recommended adapters:

```txt
pdc3d_sicol_adapter.c       world_probe -> SiCol
pdc3d_cc_adapter.c          world_probe -> CC collision
pdc3d_vphysics_adapter.c    throw/body impacts -> VPhysics
pdc3d_entity_adapter.c      actor ids -> engine entities
pdc3d_anim_adapter.c        socket_pose -> bones/sockets
pdc3d_debug_render.c        debug lines -> engine renderer
```

The key bridge methods are:

```c
pdc3d_bridge.socket_pose
pdc3d_bridge.damage_event
pdc3d_bridge.world_probe
```
