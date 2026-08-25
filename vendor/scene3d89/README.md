# scene3d89 v1.1 — Pure 3D Scene Orchestration Manager

`scene3d89` es una librería agnóstica de administración de escena para un game engine 3D C89.

No renderiza, no simula físicas, no decide IA, no carga assets. Su trabajo es mantener una verdad común de escena para el runtime: escenas, nodos, jerarquías, transforms, layers, groups, flags, bounds, queries, requests, slots, dependencies, prefabs, portales/sectores, dirty traversal y DAG semántico.

## Contrato duro

- C89 / gnu89 friendly.
- Sin `malloc`, `calloc`, `realloc`, `free`.
- Sin heap propio.
- Sin `float` ni `double`.
- Fixed-point 16.16 (`sm3d_fx`).
- Arrays internos de capacidad fija configurables por macros.
- Arena opcional provista por el usuario (`sm3d_arena`) para scratch externo.
- Agnóstica de renderer/física/audio/IA mediante callbacks y feed structs.

## Qué resuelve

```text
Game runtime
   |
   +-- scene3d89: escenas, scene tree, scene graph, DAG semántico,
   |              slots, deps, prefabs, portales, dirty update, queries
   |
   +-- renderer recibe renderables/culling hints
   +-- physics recibe collidables/bounds
   +-- AI recibe actores/markers/spawns/sectores
   +-- audio recibe anchors/volumes
   +-- scripting recibe lifecycle events
```

## Inspiración técnica

- Godot: `SceneTree`, jerarquía de nodos, escenas y grupos.
- Unity: escenas aditivas y prefabs instanciables.
- Unreal: persistent level + sublevels/streaming.
- OpenSceneGraph: visitor traversal, node masks y graph traversal.

## Estructura

```text
scene3d89_v2/
├─ include/
│  ├─ sm3d_scene.h
│  ├─ sm3d_scene_bridge.h
│  └─ sm3d_arena.h
├─ src/
│  ├─ sm3d_scene.c
│  ├─ sm3d_scene_bridge.c
│  └─ sm3d_arena.c
├─ demo/
│  └─ demo_scene3d89.c
├─ tests/
│  ├─ test_scene3d89.c
│  └─ test_scene3d89_v2.c
├─ docs/
│  ├─ DESIGN_NOTES.md
│  ├─ ENGINE_BRIDGE_PROTOCOL.md
│  ├─ SCENE_ORCHESTRATION_V2.md
│  └─ MANIFEST.txt
└─ Makefile.mingw
```

## API nueva v1.1

### Scene requests

```c
sm3d_scene_load_request(&ctx, sm3d_hash_cstr("room_a"),
                        SM3D_SCENE_SLOT_ROOM, 1,
                        SM3D_SCENE_FLAG_STREAMABLE, &request_id);

sm3d_scene_process_requests(&ctx, 1);
```

### Slots

```c
sm3d_scene_create_in_slot(&ctx, sm3d_hash_cstr("persistent"),
                          SM3D_SCENE_FLAG_PERSISTENT,
                          SM3D_SCENE_SLOT_PERSISTENT, 0, &scene_id);
```

Slots incluidos:

```text
SM3D_SCENE_SLOT_PERSISTENT
SM3D_SCENE_SLOT_LEVEL
SM3D_SCENE_SLOT_ROOM
SM3D_SCENE_SLOT_OVERLAY
SM3D_SCENE_SLOT_WORLD_CELL
SM3D_SCENE_SLOT_USER0+
```

### Dependencies entre escenas

```c
sm3d_scene_dependency_add(&ctx, level_scene, persistent_scene,
                          SM3D_DEP_REQUIRED, 0UL, 0UL);
```

Un `unload_request` falla con `SM3D_ERR_DEP_BLOCKED` si otra escena cargada depende de esa escena.

### Prefab / scene-template instancing

```c
sm3d_prefab_create(&ctx, sm3d_hash_cstr("enemy_pair"), 0UL, &prefab_id);
sm3d_prefab_add_node(&ctx, prefab_id, &node_desc, &template_index);
sm3d_prefab_instantiate(&ctx, prefab_id, scene_id, parent,
                        &base_transform, &out_root);
```

### Portal-sector visibility hints

```c
sm3d_portal_hint_add(&ctx, scene_id, portal_node,
                     sector_a, sector_b,
                     SM3D_PORTAL_FLAG_OPEN | SM3D_PORTAL_FLAG_VISIBLE,
                     SM3D_LAYER_ALL, 0UL);

sm3d_portal_collect_visible(&ctx, scene_id, sector_a, 4, &result);
```

### Dirty subtree traversal fino

```c
sm3d_node_set_local_pos(&ctx, actor, pos);
sm3d_traverse_dirty(&ctx, scene_id, SM3D_LAYER_ALL, visit_fn, user);
sm3d_update_dirty_transforms(&ctx);
```

### DAG avanzado semántico

El transform principal sigue siendo árbol estricto. El DAG vive encima como grafo semántico para dependencias, orden lógico, visibilidad, render, recursos o relaciones de sistemas.

```c
sm3d_dag_link_add(&ctx, node_a, node_b,
                  SM3D_DAG_KIND_LOGIC,
                  SM3D_DAG_FLAG_ACYCLIC, 0UL);
```

## Manifest opcional

El parser lee texto provisto por el engine. No abre archivos directamente.

```text
scene persistent persistent 0 0x0004
scene level01 level 0 0x0008
dep level01 persistent required 0
load room_a room 1 0x0010
process
```

```c
sm3d_manifest_read_text(&ctx, manifest_text, 0);
```

## Compilar

Con MinGW/MSYS2:

```sh
mingw32-make -f Makefile.mingw demo
mingw32-make -f Makefile.mingw test
mingw32-make -f Makefile.mingw test-v2
```

En Linux/clang/gcc para smoke test rápido:

```sh
gcc -std=c89 -Wall -Wextra -pedantic -Iinclude src/*.c tests/test_scene3d89_v2.c -o test_scene3d89_v2
./test_scene3d89_v2
```

## Macros de capacidad

```sh
gcc -std=c89 -DSM3D_MAX_NODES=2048 -DSM3D_MAX_SCENES=32 ...
```

Nuevas macros relevantes:

```text
SM3D_MAX_SCENE_REQUESTS
SM3D_MAX_SCENE_DEPS
SM3D_MAX_PREFABS
SM3D_MAX_PREFAB_NODES
SM3D_MAX_PORTALS
SM3D_MAX_DAG_LINKS
SM3D_MAX_DIRTY_NODES
SM3D_MAX_MANIFEST_TOKENS
```

## Estado

`phase2:scene_orchestration_manager` listo para vendorizar.
