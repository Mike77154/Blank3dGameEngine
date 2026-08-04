# Giffany Motion89: Air Lunge + Ground Lance

Dos controladores de movimiento 3D agnósticos y vendorizables:

1. **gairlunge89**: una entidad ya está en el aire y, al recibir el disparador, compromete su trayectoria hacia un punto u objetivo. Sirve para empujar, embestir, zarpear o atravesar.
2. **ggroundlance89**: una carga continua de A hacia B, rasante y adherida al suelo, con variante de lanza medieval, ariete, Puño de Pegaso o micro-saltos pegados al terreno.

## Protocolo

- C89 estricto.
- Fixed-point Q16.16.
- Sin `malloc`, `calloc`, `realloc`, `free` ni heap interno.
- Sin `float` ni `double`.
- Estado alojado por el llamador.
- API pura `input -> step -> output`.
- Adaptador opcional de providers para transform, objetivo, suelo, contacto e impacto.
- Sin dependencia de motor, render, físicas o sistema de entidades.
- Licencia CC0/public domain.

## Librería 1: gairlunge89

La activación normal exige:

- `trigger = 1`.
- `target_valid = 1`.
- `airborne = 1` cuando `require_airborne` está activo.

Intenciones disponibles:

- `GAL_INTENT_PUSH`: empuje sin daño obligatorio.
- `GAL_INTENT_RAM`: embestida corporal.
- `GAL_INTENT_CLAW`: ataque de garra/zarpazo durante el vuelo.
- `GAL_INTENT_PIERCE`: atraviesa hasta agotar `pierce_contacts`.

La biblioteca no inventa daño ni físicas. Emite `impact_event`, `impact_direction` e `impact_impulse`; el motor decide knockback, daño, stagger, ragdoll, pared rota o perforación.

## Librería 2: ggroundlance89

Estilos:

- `GGL_STYLE_LANCE`: carga recta y estable.
- `GGL_STYLE_PEGASUS`: carga rasante con pulso vertical corto.
- `GGL_STYLE_RAM`: ariete pesado.
- `GGL_STYLE_SKIM_HOP`: sucesión de micro-saltos visuales sin despegarse del terreno.

Políticas de contacto:

- `GGL_CONTACT_STOP`.
- `GGL_CONTACT_BOUNCE`.
- `GGL_CONTACT_PIERCE`.

`query_ground` permite que la carga copie desniveles. `max_step_up` y `max_step_down` evitan teletransportes verticales al cruzar bordes o escalones.

## Compilación de prueba

```sh
gcc -std=c89 -pedantic -Wall -Wextra \
  demos/demo_motion89.c \
  gairlunge89/gairlunge89.c \
  ggroundlance89/ggroundlance89.c \
  -o demo_motion89
```

## Integración recomendada

```text
Input / IA / Animación
        |
        v
  trigger de ataque
        |
        +------------------------+
        |                        |
        v                        v
 gairlunge89               ggroundlance89
 (solo en aire)            (solo rasante)
        |                        |
        v                        v
 velocidad/facing         posición/velocidad/facing
        |                        |
        +-----------+------------+
                    v
           motor de colisiones
                    |
                    v
       daño / knockback / pierce / FX
```

## Notas de unidades

Los defaults suponen **un paso lógico por tick**. Si tu motor usa otra escala, configura `speed`, `acceleration`, radios e impulsos en la misma unidad fija que el transform del engine.

La aproximación de longitud evita raíz cuadrada y enteros de 64 bits. Está pensada para gameplay, no para geometría científica. Mantén coordenadas y velocidades dentro del rango razonable de Q16.16 en 32 bits.
