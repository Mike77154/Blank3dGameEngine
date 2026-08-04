# GFO entity objects in Blank3D

Blank3D now vendors `gfo_lib` and assigns exactly one GFO runtime/context to each live entity.

## Flow

```text
player.ini / zombie.ini
  -> initializer properties
  -> one GFO instance for that entity
  -> lifecycle create / step / render / destroy
  -> DDSL2, FPIL or RPYL callback
  -> mesh provider selected by `mesh=`
```

## Entity INI

- `[entity]`: `name`, `gfo`, `hp`, `speed`
- `[logic]`: `type=ddsl2|fpi|rpyl`, `script=...`
- `[visual]`: `mesh`, `scale_x/y/z`, `draw_script`

Supported mesh selectors:

- `cube`, `sphere`, `capsule`
- `composed` for multiple primitive/model parts
- `customcalled:name` for an engine callback
- external paths ending in `.obj`, `.fbx`, `.dae`, `.gltf`, `.glb`

External paths are provider requests: the GFO object layer does not parse model formats itself.

## RPYL spawning contract

A new enemy type is data, not a C branch:

```text
spawn_entity config/entities/fast_zombie.ini 3
```

The RPYL host resolves the INI, allocates an engine entity, and calls `blank3d_objects_spawn()`.

## Important ownership rule

GFO owns lifecycle sequencing. The native engine entity owns transform, collision and rendering storage. DDSL2/FPIL/RPYL own behavior execution. Mesh providers own visual construction/loading.
