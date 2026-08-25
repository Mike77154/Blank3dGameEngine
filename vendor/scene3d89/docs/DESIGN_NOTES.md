# Design Notes — scene3d89

## Principio central

`scene3d89` no es un renderer ni un ECS completo. Es la columna vertebral de escena:

- qué existe,
- dónde está,
- a qué escena pertenece,
- quién es padre/hijo de quién,
- si está activo/visible,
- qué layer/group/tipo tiene,
- qué bounds expone para queries,
- cómo se lo entregas a render/física/IA/audio/scripting.

## Node graph

Cada nodo usa índices internos y handles generacionales. Eso evita que un sistema guarde un handle viejo y lo use después de destruir/reusar el slot.

```text
Scene Root
├─ Sector / Room
│  ├─ Actor
│  │  ├─ Mesh Ref
│  │  ├─ Attach Point
│  │  └─ Camera Anchor
│  ├─ Trigger Volume
│  └─ Portal
└─ Spawn Point
```

## Transform propagation

Se conserva transform local y world.

- Posición world: parent.world.pos + local.pos escalado por parent.scale.
- Rotación world: suma euler fixed-point.
- Escala world: multiplicación fixed-point.

No hay matrices ni trigonometría para mantener el kernel pequeño y portable. El renderer puede convertir a matriz real si quiere.

## Layers y groups

- `layer_mask`: para render/física/audio/cámara/etc.
- `group_mask`: para gameplay semántico: enemigos, pickups, puertas, triggers.

Esto copia la idea práctica de node masks/grupos sin acoplarse a un engine específico.

## Scenes

Cada escena tiene root node propio. Puedes manejar:

- escena persistente,
- escenas aditivas,
- escenas streamables,
- escenas pausadas/inactivas.

La librería no lee archivos ni streaméa disco; solo expone estado para que tu engine lo haga.

## Queries

Incluye dos caminos:

1. Query lineal por flags/layers/AABB.
2. Grid XZ opcional de capacidad fija para mundos más grandes.

## Threading / mutations

No intenta ser multithread real. Tiene `begin_read/end_read` para proteger traversal de mutaciones accidentales. Si se quiere multi-thread real, el engine debe poner el lock externo.
