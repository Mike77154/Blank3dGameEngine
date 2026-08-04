# Ordnance Integration Report — v1.3.0

## Módulos

### `gbulletair89 v1.2`

Proveedor de pasos balísticos, whiz, zip y cracks. Se registra en una clase de
voz de detalle balístico y puede avanzar virtualmente. El bridge convierte su
salida fija de 48 kHz a la frecuencia del engine mediante fase Q16 e interpolación
lineal entera.

### `wsound_ggrenadeblast89 v1.0.2`

Proveedor de explosiones de granadas. Conserva su propia cadena de EQ,
distorsión, chorus y reverb, pero se presenta al Voice Handler como una sola voz
crítica caller-owned.

### `wsound_rocketblast89 v1.3.0`

Proveedor de cohete y blast. Su salida interna estéreo se convierte a una fuente
mono estable para que el Voice Handler controle paneo, distancia y prioridad de
forma uniforme con el resto del motor.

## Política de voz

- bullets: detalle balístico, virtualización permitida;
- grenades: explosión crítica, `NEVER_VIRTUAL` durante el evento;
- rockets: explosión crítica, `NEVER_VIRTUAL` durante el evento;
- todos los pools son proporcionados por el caller;
- si no existe un slot de proveedor compatible, el evento se rechaza limpiamente
  sin tocar memoria externa.

## Tamaños medidos

```text
gsso89_context                 56 B
gsso89_bullet_voice         4,904 B
gsso89_grenade_voice       33,352 B
gsso89_rocket_voice        48,552 B
libgsynthordnance89.a      62,694 B
```

Pool sugerido 32 bullets + 4 grenades + 2 rockets:

```text
32 * 4,904   = 156,928 B
 4 * 33,352  = 133,408 B
 2 * 48,552  =  97,104 B
context      =      56 B
TOTAL        = 387,496 B (~378.4 KiB)
```

## Validación

- smoke test inicia las tres familias: PASS;
- C89 `-pedantic -Wall -Wextra -Werror`: PASS;
- objetos i386 de todos los cores: PASS;
- forbidden-token scan de cores nuevos/modificados: PASS;
- todos los WAV: 44.1 kHz, estéreo, PCM16, cero clipping.
