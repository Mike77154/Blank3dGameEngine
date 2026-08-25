# INTEGRATION

## Pipeline recomendado

```text
Input DSL / Player / IA
        │
        ▼
GLOCO_Input
        │
        ▼
gloco_update_actor()
        │
        ├── world_probe callback ──► tu collision/sweep/ground solver
        │
        ├── event callback ────────► sonidos, footstep, dust, anim notify
        │
        ▼
GLOCO_RenderPose
        │
        ▼
Renderer / Animator / Entity Transform
```

## Camara agnostica

Para FPS o TPS usa:

```c
in = gloco_bridge_make_camera_input(move_x, move_z, buttons, cam_fwd, cam_right);
```

Para tanque, enemigo, aliado o NPC usa:

```c
in = gloco_bridge_make_actor_input(move_x, move_z, buttons, actor_fwd, actor_right);
```

La libreria no sabe si el input viene de jugador, IA, script o replay.

## World probe minimo

```c
static int my_probe(void *user,
                    const GLOCO_Vec3 *from,
                    const GLOCO_Vec3 *to,
                    GLOCO_FX radius,
                    GLOCO_FX height,
                    GLOCO_ProbeResult *out_result)
{
    out_result->corrected_pos = *to;
    out_result->ground_normal = gloco_v3(0, GLOCO_FX_ONE, 0);
    out_result->flags = 0;

    if (out_result->corrected_pos.y <= 0) {
        out_result->corrected_pos.y = 0;
        out_result->flags |= GLOCO_FLAG_GROUNDED;
    }

    return 1;
}
```

Para version real, sustituye el piso plano por tu capsule sweep:

1. Barrer capsule desde `from` hasta `to`.
2. Si pega pared, deslizar contra plano.
3. Ray/sweep hacia abajo con `ground_snap`.
4. Si normal del suelo es valida, set `GLOCO_FLAG_GROUNDED`.
5. Si normal es muy inclinada, set `GLOCO_FLAG_STEEP`.
6. Si el desplazamiento se bloquea, set `GLOCO_FLAG_BLOCKED`.

## Animacion

`GLOCO_RenderPose` entrega:

- `state`: idle/walk/run/sprint/aim_move/evade/slide/air.
- `velocity`: para blendspace.
- `facing`: para torso/feet orientation.
- `stride_phase`: 0..1 fixed-point.
- eventos `GLOCO_EVENT_STEP_L/R`: para footstep, polvo, casquillos, cam shake sutil.


## Provider mode opcional

Para integraciones mas profundas, `gloco89` ahora admite dos providers independientes:

```text
Input / IA / DSL
      |
      v
 gloco89 policy/state
      |
      +--> GLOCO_MovementProvider
      |       horizontal / vertical / impulse / integrate
      |
      +--> GLOCO_PhysicsProvider
              move / lifecycle / teleport / post_move
```

No es obligatorio usarlos. Si no hay provider, o si un callback devuelve `GLOCO_PROVIDER_FALLBACK`, se ejecuta el comportamiento original. El viejo `GLOCO_WorldProbeFn` permanece soportado. Ver `PROVIDERS.md`.
