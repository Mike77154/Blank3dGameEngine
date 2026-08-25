# Monika WOFF1 Decoder C89

Decoder WOFF1 -> SFNT (`.ttf` / `.otf`) hecho en C89 con buffers dados por el llamador y límites estáticos configurables.

## Qué trae

- Parser de header WOFF1 de 44 bytes.
- Parser de directorio de tablas WOFF1.
- Reconstrucción SFNT:
  - `sfntVersion` / flavor original.
  - `numTables`.
  - `searchRange`, `entrySelector`, `rangeShift` recalculados.
  - Directorio de tablas SFNT ordenado por `tag`.
  - Datos de tablas escritos con alineación de 4 bytes.
- Inflador zlib/DEFLATE interno:
  - Bloques stored.
  - Huffman fijo.
  - Huffman dinámico.
  - Adler-32.
- Validaciones:
  - Signature `wOFF`.
  - Campo `reserved` en cero.
  - `length` del header contra tamaño real.
  - `compLength <= origLength`.
  - Offsets y rangos dentro del archivo.
  - Solapamiento de tablas / metadata / private data.
  - Tags ordenados ascendentemente.
  - `totalSfntSize` calculado contra header.
  - Checksum por tabla.
- Reparación de `checkSumAdjustment` en `head` por defecto.
- Extracción opcional de metadata XML y private data.

## Build

```sh
make
```

Compilación estricta usada durante la prueba:

```sh
cc -std=c89 -pedantic -Wall -Wextra -O2 \
  -o woff1dec \
  src/woff1_decoder.c src/woff1_inflate.c src/woff1_cli.c
```

## Uso

```sh
./woff1dec input.woff output.ttf
./woff1dec input.woff output.otf --meta metadata.xml --priv private.bin
```

El tipo de salida real depende del `flavor` del WOFF:

- `0x00010000` normalmente TrueType outlines.
- `OTTO` normalmente OpenType/CFF outlines.

## Límites estáticos

Edita `src/woff1_config.h` o compila con defines:

```sh
cc -DWOFF1_MAX_TABLES=256 \
   -DWOFF1_MAX_WOFF_SIZE=67108864ul \
   -DWOFF1_MAX_SFNT_SIZE=134217728ul \
   ...
```

## API mínima

```c
#include "woff1_decoder.h"

static unsigned char in_buf[WOFF1_MAX_WOFF_SIZE];
static unsigned char out_buf[WOFF1_MAX_SFNT_SIZE];

woff1_u32 out_len;
woff1_report report;
int r;

r = woff1_decode_to_sfnt(in_buf, in_len,
                         out_buf, WOFF1_MAX_SFNT_SIZE,
                         &out_len, &report);
```

## Tests

```sh
make test_inflate
make
./woff1dec tests/sample_compressed.woff tests/sample_compressed.ttf
```

La muestra de prueba se generó para cubrir tablas comprimidas y reconstrucción SFNT.

## Notas técnicas

- El decoder no depende de una biblioteca zlib externa.
- No usa aritmética real; todo es entero.
- El CLI usa `stdio` solo para leer y escribir archivos. La librería principal opera con punteros y tamaños.
- Si reordenas tablas o reparas checksum global, una tabla `DSIG` de una fuente real puede quedar invalidada, como suele pasar en conversiones SFNT.
