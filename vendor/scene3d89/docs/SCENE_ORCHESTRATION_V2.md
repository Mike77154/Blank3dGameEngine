# scene3d89 v1.1 — Scene Orchestration Notes

## Principio central

`scene3d89` no intenta ser renderer, physics engine, asset loader ni ECS completo. Es la capa de organización de escena.

La arquitectura queda así:

```text
Transform tree                    DAG overlay
-------------                    -----------
Padre/hijo estricto              Relaciones no-transform
sin ciclos                       opcionalmente acíclicas
world transform                  dependencias lógicas
bounds                           orden de systems
reparent seguro                  recursos/visibilidad/render hints
```

El árbol resuelve transforms. El DAG resuelve relaciones semánticas.

## Scene requests

La cola de requests permite que el engine pida cargar/descargar escenas sin acoplar a filesystem o threads.

```c
sm3d_scene_load_request(...);
sm3d_scene_unload_request(...);
sm3d_scene_process_requests(...);
```

El engine puede procesar `max_ops` por frame para mantener control de presupuesto.

## Scene slots

Slots recomendados:

```text
persistent: game state global, bootstrap, servicios
level: nivel principal actual
room: habitaciones/interiores streamables
overlay: UI 3D, debug, pause, combat overlay
world_cell: chunks grandes de mundo
```

## Dependencies

`sm3d_scene_dependency_add(from, to, kind, flags, tag)` declara que una escena necesita otra.

- `SM3D_DEP_REQUIRED`: bloquea unload del target si el source está cargado.
- `SM3D_DEP_WEAK`: referencia informativa, no bloquea.
- `SM3D_DEP_LOAD_BEFORE`: pista para orquestador externo.
- `SM3D_DEP_VISIBILITY`: pista de culling/streaming.

## Prefabs

Los prefabs son templates internos de nodos. No contienen assets, solo descripciones de nodos.

Cada `SM3D_PrefabNodeDesc` guarda:

```text
parent_index
type
name_hash
flags
layer/group masks
resource/archetype/user ids
local transform
local bounds
```

Instanciar un prefab crea nodos reales en una escena.

## Portal-sector visibility hints

Los portales no hacen render. Solo devuelven sectores potencialmente visibles desde un sector inicial.

Uso típico:

```text
Camera sector -> portal_collect_visible -> renderer/occlusion/culling
```

## Dirty traversal fino

Cuando un nodo se mueve, entra a una cola dirty fija. Así el engine puede:

1. recorrer solo dirty nodes;
2. alimentar renderer/physics con actualizaciones parciales;
3. actualizar transforms solo donde hubo cambios.

Si la cola se llena, `sm3d_update_dirty_transforms()` cae a update completo seguro.

## DAG avanzado

El DAG acepta links por tipo:

```text
generic
resource
logic
render
visibility
dependency
user0+
```

Con `SM3D_DAG_FLAG_ACYCLIC`, la librería rechaza ciclos.

Ejemplos:

```text
Logic order:       Trigger -> Door -> Cutscene
Resource deps:     Actor -> Mesh -> Material
Visibility hints:  Sector -> Portal -> Sector
Render order:      Mirror -> ReflectedActors
```

## Manifest

El parser es intencionalmente chico y opcional:

```text
scene persistent persistent 0 0x0004
scene level01 level 0 0x0008
dep level01 persistent required 0
load room_a room 1 0x0010
process
```

El engine decide si lee un archivo, asset pack, RPY, DSL o datos embebidos; `scene3d89` solo recibe `const char *`.
