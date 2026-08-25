# GLOCO89 locomotion provider integration - Blank3D v3.27.3

## Rol

GLOCO89 se integra como controlador/politica de locomocion. No reemplaza ThingSystem, ECS, ActorSystem, World3D, Scene3D, Soquete, MovementBaseVerbs, GAutomotion, VerticalMotion, VPhysics, MotionAttack ni los sistemas de animacion/attachment.

```text
DDSL2 / GFO / FPIL / RPYL / Input
                |
                v
      MovementBaseVerbs89
                |
        +-------+-------+
        |               |
        v               v
    GAutomotion       GLOCO89
   desired path    locomotion policy
        |               |
        +-------+-------+
                |
        +-------+--------+
        |       |        |
        v       v        v
   Vertical89 Collision VPhysics
        +-------+--------+
                |
                v
          final Transform
                |
        +-------+-------+
        v       v       v
      World    Scene  Soquete T0
```

Las maniobras 3D/especiales permanecen con sus sistemas especializados. MotionAttack puede suspender temporalmente GLOCO para evitar dos autoridades de movimiento simultaneas.

## Identidad

La identidad canonica sigue siendo:

```text
GFO Object -> Thing -> ECS -> ActorRef
```

`gloco_id` es solo un slot interno del controlador. `Blank3DGlocoBinding` asocia `Thing owner + Actor ID + gloco slot + Transform + VerticalBody`.

## Transform

El Transform del host es la autoridad espacial final. GLOCO calcula y solicita movimiento; Collision/VPhysics/VerticalAxis resuelven el resultado. Despues del movimiento se sincronizan Runtime Spine, World3D, Scene3D y Soquete T0.

## Stamina / NumSys

Blank3D instala `GLOCO_StatsProvider` y registra `locomotion.stamina` en NumSys por Thing.

Conversion:

```text
GLOCO Q8.8 <-> NumSys Q10
```

Si el provider no puede atender la operacion, GLOCO conserva su fallback interno.

## VarRuntime

Blank3D registra un provider de variables GLOCO con prioridad inferior a NumSys. Por ello `locomotion.stamina` sigue perteneciendo a NumSys, mientras los parametros del perfil y el estado vivo pueden tocarse/consultarse por el authoring universal.

Ejemplos:

```text
locomotion.run_speed = 6
locomotion.acceleration += 2
locomotion.evade_stamina_cost = 18
```

Lecturas live de solo lectura:

```text
locomotion.grounded
locomotion.blocked
locomotion.steep
locomotion.evading
locomotion.sprinting
locomotion.aiming
locomotion.sliding
locomotion.state
locomotion.flags
locomotion.last_event
locomotion.event_value
```

Cada Thing ligado tiene su propio perfil runtime; editar un enemigo no muta el perfil del player ni el de los otros Things.

## Flags publicados

Para compatibilidad con el authoring existente se reflejan estados principales del player hacia FlagStore, entre ellos:

```text
player.grounded
player.sprinting
player.evading
player.sliding
```

GLOCO conserva sus flags internos transitorios; FlagStore es una publicacion para otros sistemas, no un reemplazo del estado interno del controlador.

## INI

Perfiles del host:

```text
config/locomotion/player.ini
config/locomotion/enemy.ini
```

El parser acepta `[locomotion]` o `[profile]`, presets y overrides de los parametros del perfil usando conversion decimal manual a Q8.8, sin `atof`, `float` ni `double`.

## Fixed point

- GLOCO: Q8.8
- Transform/Collision Blank3D: Q20.12 / Q12 en interfaces host
- VarRuntime: Q16.16
- NumSys: Q10

Las conversiones viven en el adapter `blank3d_gloco.c`; no se obliga a los vendors a compartir formato interno.

El sweep horizontal usa multiplicacion fraccionaria dividida por cociente/resto para no depender de un `long` de 64 bits en MinGW32.

## Provider boundaries

- MovementBaseVerbs -> GLOCO: intencion semantica walk/run/strafe.
- GAutomotion -> GLOCO: direccion/velocidad deseada para rutas flat.
- VerticalMotion -> GLOCO MovementProvider: autoridad de Y/grounded.
- Collision/VPhysics -> GLOCO PhysicsProvider: resolucion del movimiento horizontal y espejo fisico.
- NumSys -> GLOCO StatsProvider: stamina.
- VarRuntime -> GLOCO profile/state provider: live authoring.
- MotionAttack -> suspend GLOCO: override temporal de movimientos especiales.

## Invariantes

1. GLOCO nunca se convierte en identidad canonica.
2. GLOCO nunca reemplaza World/Scene/Soquete.
3. GLOCO no absorbe MovementBaseVerbs, GAutomotion, VerticalMotion o VPhysics.
4. El Transform final pertenece al host.
5. Providers opcionales mantienen fallback standalone.
6. C89, sin heap y sin floating point.
