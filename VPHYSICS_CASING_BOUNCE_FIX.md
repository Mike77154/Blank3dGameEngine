# VPhysics Casing Bounce Fix — Blank3D v3.22.1

## Síntoma

Los casquillos expulsados por GWeapon89 llegaban al suelo y permanecían
rotando como trompos. El movimiento parecía una animación fija, no una pieza de
latón con masa, impacto y pérdida de energía.

## Causa encontrada

El camino anterior en `monika_blank3d.c` era una simulación manual:

```text
gravedad por frame
+ rotación fija de 360/210 grados por segundo
+ clamp de Y contra el piso
+ inversión parcial de velocidad vertical
```

La rotación no se amortiguaba ni tenía estado de reposo. Al tocar el suelo se
corregía la posición, pero el giro seguía siendo impuesto cada frame.

Durante la migración apareció además un problema en el Total Solver:

1. La velocidad objetivo de restitución se recalculaba en cada iteración PGS.
   La primera iteración producía rebote y las siguientes lo reducían otra vez a
   velocidad normal cero.
2. Cada pequeño impulso de contacto reiniciaba `sleepCounter`, de modo que un
   cuerpo apoyado jamás podía dormir.
3. El umbral de restitución era demasiado bajo para props pequeños y permitía
   microrebotes persistentes.

## Flujo nuevo

```text
GWP89_EVENT_CASING_EJECTED
          │
          ├── actor_id -> player/enemy/ally/generic axes
          ├── origin -> ejection socket/event position
          └── casing_mesh_id -> physical profile
                           │
                           ▼
              Blank3DCasingPhysics
                           │
                           ▼
              único Blank3DVPhysics world
                           │
              ┌────────────┴────────────┐
              ▼                         ▼
       transform provider        collision provider
       quaternion + position     CCS/SICOL world
              │                         │
              └────────────┬────────────┘
                           ▼
                     renderer shell
```

## Perfil de una shell

Cada shell crea un `dynamic box` con dimensiones y masa según su mesh:

- pistol brass;
- rifle/machine-gun brass;
- shotgun shell;
- magnum brass;
- sniper/long-rifle brass.

Parámetros comunes:

```text
friction       0.65
restitution    0.45–0.55
linear damping 0.35
angular damping 3.20
pair collision false
CCD            false
VPhysics debug false
lifetime       3 s
```

Shell-shell se desactiva para evitar el coste cuadrático y el ruido de una nube
de casquillos. World-shell permanece activo.

## Correcciones físicas

### Contacto de caja orientada

El soporte vertical ya no usa solamente `half_extents.y`. Se calcula desde la
orientación quaternion y las tres semiextensiones. El punto de contacto se
coloca en la superficie de soporte, no en el centro del cuerpo; así el impulso
puede intercambiar momento lineal y angular.

### Restitución estable

`vpContact.restitutionTarget` captura la velocidad de separación deseada una
sola vez antes del warm-start y las iteraciones. Todas las iteraciones resuelven
contra el mismo objetivo.

El umbral de rebote quedó en 0.5 unidades/s: impactos visibles rebotan; contactos
lentos se convierten en reposo en vez de vibrar.

### Sleep alcanzable

`vp_wake_pair()` despierta cuerpos que estaban dormidos, pero ya no borra el
contador de cuerpos que ya estaban despiertos. Las pequeñas correcciones de un
contacto estable pueden acumular los frames necesarios para sleep.

## Ejes universales por actor

La expulsión no está fijada al player:

```text
player      -> cámara/socket del player
enemy       -> transform real del enemigo por actor_id
ally/generic-> dirección del evento y world-up
```

Por tanto, el `gunner_enemy` y cualquier actor equipado mediante GAttach pueden
expulsar shells desde su propia orientación.

## Fallback

Si VPhysics no puede crear un cuerpo por falta de slots, el camino manual sigue
existiendo como fallback, pero ahora amortigua el giro y corta el rebote al caer
por debajo de un umbral. No vuelve a imponer rotación perpetua.

## Archivos

```text
src/blank3d_casing_physics.h
src/blank3d_casing_physics.c
src/blank3d_vphysics.h
src/blank3d_vphysics.c
src/monika_blank3d.c
tests/test_casing_vphysics.c
vendor/vphysics_total_solver_c89/include/vp_contact.h
vendor/vphysics_total_solver_c89/include/vp_config.h
vendor/vphysics_total_solver_c89/src/vp_contact.c
vendor/vphysics_total_solver_c89/src/vp_solver.c
```

## Prueba

```bash
make test-casing-physics
```

La prueba exige:

- impulso ascendente inicial;
- fase descendente;
- rebote vertical real;
- reducción de velocidad angular;
- sleep final;
- matriz visual válida;
- liberación limpia del body.
