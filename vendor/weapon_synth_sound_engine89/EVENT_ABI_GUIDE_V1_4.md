# Event ABI Guide — weapon_synth_sound_engine89 v1.4.0

## Contrato general

```c
int wsse89_dispatch(wsse89_context *ctx,
                    const wsse89_event *event,
                    gv89_handle *out_handle);
```

También puede almacenarse como callback neutral:

```c
typedef int (*wsse89_event_sink_fn)(void *user,
                                     const wsse89_event *event,
                                     gv89_handle *out_handle);
```

El host mantiene `user = wsse89_context*` y usa `wsse89_event_sink` como implementación predeterminada. No hay referencias a entidades, nodos, componentes, escenas, transforms ni tipos propios de un engine.

## Reglas ABI

- `seed == 0`: el contexto genera una semilla determinista.
- Q15 sin signo: `0..32767`.
- Pan Q15 con signo: `-32768..32767`.
- Velocidad Q16: `65536 == 1.0`.
- `out_handle` puede ser `NULL` para eventos que no producen voz controlable.
- Los handles pertenecen al contexto que los produjo.
- El host debe serializar las mutaciones del contexto respecto del callback de audio; la biblioteca no crea hilos ni locks.

## Eventos y payloads

| Evento | Payload de `event.data` | Produce handle | Uso típico |
|---|---|---:|---|
| `REPORT` | `gssr89_params report` | Sí | disparo completo procedural |
| `CASING` | `gsse89_casing_params casing` | Sí | caída/rebote de casquillo |
| `FIRE_START` | `gsse89_fire_params fire` | Sí | fuego o lanzallamas continuo |
| `BULLET_PASS` | `gsso89_bullet_params bullet` | Sí | bala pasando cerca |
| `GRENADE_BLAST` | `gsso89_grenade_params grenade` | Sí | explosión de granada |
| `ROCKET_BLAST` | `gsso89_rocket_params rocket` | Sí | explosión de cohete; preset o `wsrb89_params` completos |
| `PROJECTILE` | `gssw89_projectile_params projectile` | Sí | firma de proyectil/world |
| `IMPACT` | `gssw89_impact_params impact` | Sí | impacto por material |
| `RICOCHET` | `gssw89_ricochet_params ricochet` | Sí | ricochet procedural |
| `STOP_HANDLE` | `wsse89_stop_event stop` | No | detener voz con release |
| `SET_HANDLE_SPATIAL` | `wsse89_spatial_event spatial` | No | pan, distancia, oclusión y foco |
| `EXPANSION_SHOT` | `wsse89_expansion_shot_event expansion_shot` | No | reacción de muzzle device |
| `BELT_START` | `wsse89_belt_event belt` | No | iniciar alimentación por cinta |
| `BELT_STOP` | sin payload obligatorio | No | detener alimentación por cinta |
| `FRICTION_START` | `wsse89_friction_event friction` | No | fricción continua por material |
| `FRICTION_STOP` | sin payload obligatorio | No | detener fricción |
| `AERO_START` | `wsse89_aero_event aero` | No | viento/proyectil/cámara cercana |
| `AERO_STOP` | sin payload obligatorio | No | detener aero |
| `PARTICLES` | `wsse89_particles_event particles` | No | grava, debris o partículas |
| `AMMO` | `wsse89_ammo_event ammo` | No | munición suelta/cargador |
| `THERMAL_HEAT` | `wsse89_thermal_event thermal` | No | pops y tensión térmica |
| `LISTENER_EXPOSE` | `wsse89_listener_event listener` | No | exposición/protección auditiva |
| `MASK_TRIGGER` | `mask_energy_q15` | No | máscara/transiente de mezcla |
| `SPATIAL_AZIMUTH` | `wsse89_spatial_azimuth_event azimuth` | No | pan/ITD/sombra global |
| `PORTAL_PATH` | `wsse89_portal_event portal` | No | ruta acústica por puerta/ventana |
| `OUTDOOR_MIX` | `wsse89_outdoor_mix_event outdoor_mix` | No | envío y wet exterior |
| `ACTION_START` | `wsse89_action_event action` | No | pump, bolt, slide, feed, etc. |
| `RECEIVER_EXCITE` | `receiver_impulse` | No | excitar resonancia del arma |

## Mapeos sugeridos

```text
Unity/Godot/Unreal/engine propio     WSSE89
---------------------------------------------------------------
OnWeaponFired                     -> REPORT + EXPANSION_SHOT
OnProjectileNearListener          -> BULLET_PASS o PROJECTILE
OnCollision(material, velocity)   -> IMPACT
OnRicochet                        -> RICOCHET
OnExplosion                       -> GRENADE_BLAST/ROCKET_BLAST
OnReloadPhase                     -> ACTION_START / API mecánica directa
OnTransformChanged                -> SET_HANDLE_SPATIAL
OnRoomPortalChanged               -> PORTAL_PATH
OnAudioListenerProtectionChanged  -> LISTENER_EXPOSE
OnWeaponDestroyed/Stopped         -> STOP_HANDLE
```

## Callback de audio

Por bloque intercalado estéreo:

```c
(void)wsse89_render_stereo(&audio, output_pcm16, frames, 0);
```

Por muestra:

```c
wsse89_process_stereo_sample(&audio, &left, &right);
```

`accumulate != 0` suma sobre un buffer existente usando el mezclador/limitador interno.

## Control avanzado

Los módulos completos siguen accesibles por `ctx.reports`, `ctx.base`, `ctx.ordnance`, `ctx.expansion`, `ctx.world` y `ctx.handler`. El bus de eventos no oculta ni elimina las APIs originales vendorizadas.

## Control fino del cohete v1.4.1

`gsso89_rocket_params` incluye `synth_params` y `use_custom_params`. El host puede llamar `gsso89_rocket_load_preset`, modificar niveles/decays/SVF/LFO y finalmente `gsso89_rocket_enable_custom`. El mismo payload viaja por `WSSE89_EVENT_ROCKET_BLAST`; no se requiere una ruta especial del game engine.
