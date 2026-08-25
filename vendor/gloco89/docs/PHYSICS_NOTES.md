# PHYSICS_NOTES

## Idea central

La locomocion moderna se siente bien cuando el actor no salta instantaneamente a una velocidad final. En lugar de eso:

1. Se calcula direccion deseada desde input y basis de camara/actor.
2. Se calcula velocidad objetivo segun estado: walk, run, sprint, aim, crouch.
3. La velocidad actual se mueve hacia esa velocidad objetivo usando aceleracion.
4. Si no hay input, braking y friction reducen la velocidad.
5. El estado temporal de evade/slide puede bloquear o reducir control del jugador.
6. El world probe externo corrige posicion contra suelo, slopes y paredes.

## Nombres de fisica incluidos

| Nombre | Funcion |
|---|---|
| acceleration envelope | Curva discreta para llegar a la velocidad objetivo. |
| braking deceleration | Frenado constante sin input. |
| proportional ground friction | Damping proporcional a velocidad horizontal. |
| camera-relative locomotion | Movimiento por basis de camara, actor o mundo. |
| strafe/backpedal scaling | Penalizacion lateral/atras para movimiento tactico. |
| sprint stamina gate | El sprint depende de stamina minima y drenaje. |
| evade impulse window | Impulso de esquiva con ventana activa y recovery. |
| slide friction window | Deslizamiento temporal con friccion especifica. |
| floor snap bridge | El motor externo mantiene o suelta suelo segun geometria. |
| stride telemetry | Eventos de paso para animacion, sonido, polvo o huellas. |

## Por que callback de mundo

Un buen controller no debe asumir si el juego usa BSP, triangle soup, heightmap, capsules, voxel o un backend fisico. `gloco89` solo pide un `world_probe`:

```c
typedef int (*GLOCO_WorldProbeFn)(void *user,
                                  const GLOCO_Vec3 *from,
                                  const GLOCO_Vec3 *to,
                                  GLOCO_FX radius,
                                  GLOCO_FX height,
                                  GLOCO_ProbeResult *out_result);
```

Tu motor puede hacer capsule sweep, raycast triple, step solver o cualquier mezcla.

## Fixed point

- `GLOCO_FX_ONE = 256`.
- `2.0` se escribe como `gloco_fx_from_int(2)`.
- Los perfiles usan unidades por segundo en Q8.8.
- `dt_ms` entra como milisegundos enteros.

## Limites intencionales

Esta version no trae:

- IK de pies.
- Root motion importado.
- Network prediction.
- Collision sweep interno contra triangulos.
- Animacion completa.

Trae lo necesario para que esas capas se conecten encima sin ensuciar el kernel.
