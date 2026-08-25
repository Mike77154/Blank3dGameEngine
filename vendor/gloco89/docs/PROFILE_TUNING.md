# PROFILE_TUNING

## Perfiles incluidos

| Perfil | Sabor |
|---|---|
| `GLOCO_PROFILE_DEFAULT` | Balanceado. |
| `GLOCO_PROFILE_TACTICAL` | Pesado/controlado, ideal para shooter con aim/strafe. |
| `GLOCO_PROFILE_ARCADE` | Rapido, respuesta fuerte, mas videojuego. |
| `GLOCO_PROFILE_HEAVY` | Tanque, enemigo grande o personaje con peso. |

## Campos clave

```c
p.walk_speed;
p.run_speed;
p.sprint_speed;
p.aim_speed;
p.acceleration;
p.sprint_acceleration;
p.braking;
p.hard_braking;
p.ground_friction;
p.side_scale;
p.back_scale;
p.evade_speed;
p.evade_active_ms;
p.evade_recover_ms;
p.slide_min_speed;
p.slide_friction;
```

## Recetas rapidas

### Shooter tactico

- Baja `side_scale` y `back_scale`.
- Sube `ground_friction`.
- Sube `braking`.
- Evade corto y caro en stamina.

### Shooter arcade

- Sube `acceleration`.
- Baja `ground_friction`.
- Sube `sprint_speed`.
- Evade rapido con recovery corto.

### Enemigo pesado

- Baja velocidades.
- Baja aceleracion.
- Sube friccion.
- Evade lento o desactivado por stamina/coste alto.

## Nota sobre unidades

Todo es Q8.8. Ejemplo:

```c
p.run_speed = gloco_fx_from_int(5);       /* 5 unidades/s */
p.side_scale = GLOCO_FX_ONE * 80L / 100L; /* 0.80 */
```
