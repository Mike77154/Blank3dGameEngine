# PCX89 — saneamiento C89 estático

Esta variante conserva las rutas de parsing, decodificación RGB/indexada, paletas, inspección, diagnóstico y encoding PCX del codec recibido, pero cambia el modelo de almacenamiento para ajustarse a un runtime C89 de recursos fijos.

## Contrato técnico

- C89 estricto.
- Matemática entera exacta. PCX no necesita operaciones fraccionales en estas rutas, así que no se introduce aproximación; cualquier extensión que necesite fracciones debe usar fixed-point.
- `pcx_size` y `pcx_u32` son `unsigned int` y existe una comprobación de compilación que exige 32 bits.
- `pcx_off` es `int`; el codec limita su dominio de trabajo a tamaños compatibles con esos contadores.
- No se usa almacenamiento dinámico en los `.c/.h` de producción.
- Encoder, imagen RGB, imagen indexada, scanline y reportes usan buffers estáticos de capacidad configurable en `pcx_config89.h`.

## Capacidades estáticas por defecto

- RGB/image store: 16 MiB.
- Indexed store: 8 MiB.
- Encoder output: 32 MiB.
- Scanline scratch: 256 KiB.
- Row-index scratch: 64 KiB.
- Report scratch: 16 KiB.

Se pueden cambiar en `pcx_config89.h` antes de compilar. Si una operación excede la capacidad configurada, devuelve `PCX_ERR_LIMITS` en lugar de intentar crecer memoria en runtime.

## Cambio de ownership

Las funciones de carga/encoding de conveniencia devuelven punteros a almacenamiento estático del codec. El contenido se considera válido hasta que otra operación que reutilice ese mismo store lo sobrescriba. Para datos que deban persistir, cópialos a un buffer fijo propiedad de tu engine. Las helpers `pcx_image_use_buffer` y `pcx_indexed_image_use_buffer` sirven para describir buffers externos sin transferir ownership.

## Compilación

```sh
make
```

Genera `libpcx89.a`.

## Verificación diferencial

`verification/bytecmp_results.json` contiene el resultado detallado contra el codec original recibido.

- 204/204 comparaciones de decodificación iguales: rutas de archivo y memoria, RGB e indexado/paleta.
- 6/6 comparaciones de encoding iguales: RGB RLE, RGB raw e indexado 1/2/4/8 bpp.
- En cada caso se compararon tanto metadata/resultado como payload completo byte por byte.
