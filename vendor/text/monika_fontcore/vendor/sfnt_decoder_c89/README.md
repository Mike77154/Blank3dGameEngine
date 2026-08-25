# sfnt_decoder_c89

Decoder/lector SFNT minimalista y bastante práctico para fonts TrueType/OpenType, escrito en C89.

## Reglas del paquete

- Sin `malloc`, `free`, `realloc`, ownership de heap ni buffers internos gigantes escondidos.
- Sin `float` ni `double`; las matrices de glyph compuesto usan fixed-point 16.16.
- La librería no hace I/O: tú le pasas `const void *font_bytes` + tamaño.
- El ejemplo `sfntdump` usa un buffer `static` de archivo para probar desde consola.
- No incluye archivos `.ttf`, `.otf` ni ninguna font embebida.

## Qué decodifica

- Offset table / table directory SFNT.
- TTC básico: abre una colección `ttcf` por índice.
- Búsqueda segura de tablas por tag.
- Checksums de tabla y checksum global del font.
- `head`: unitsPerEm, bbox, loca format.
- `maxp`: numGlyphs.
- `hhea` / `hmtx`: métricas horizontales por glyph.
- `cmap`: selección y lookup Unicode para formatos 0, 4, 6, 12 y 13.
- `loca` / `glyf`: rango raw de glyph, header, decode de glyph simple y visitor de componentes compound.
- `name`: iteración raw de name records.
- `OS/2`: métricas básicas.
- Detecta `CFF ` y `CFF2`, pero este paquete no ejecuta charstrings CFF/CFF2.

## API mínima

```c
sfnt_face face;
sfnt_u16 gid;
int r;

r = sfnt_open(&face, font_bytes, font_size);
if (r != SFNT_OK) { /* manejar error */ }

r = sfnt_lookup_glyph(&face, 0x0041u, &gid); /* Unicode 'A' */
```

Para outlines TrueType simples:

```c
static sfnt_point points[4096];
sfnt_outline outline;

r = sfnt_decode_simple_glyph(&face, gid, points, 4096u, &outline);
if (r == SFNT_OK) {
    /* points[i].x/y, points[i].on_curve, points[i].end_contour */
}
```

Para compound glyphs:

```c
static int visit(void *user, const sfnt_component *c) {
    /* c->a,b,c,d son 16.16; c->arg1/arg2 pueden ser XY o point indices según flags */
    return SFNT_OK;
}

r = sfnt_visit_compound_glyph(&face, gid, visit, 0);
```

## Compilar

```sh
make
make check
```

O directo:

```sh
gcc -std=c89 -pedantic -Wall -Wextra -O2 -Iinclude \
    src/sfnt_decoder.c examples/sfntdump.c -o sfntdump
```

## Probar con una font tuya

```sh
./sfntdump /ruta/a/tu/font.ttf
./sfntdump /ruta/a/tu/font.ttf 1F600
```

## Integración estilo engine retro

La librería está pensada para integrarse con un loader que ya tenga el font en ROM/asset-pack/memoria estática. No intenta rasterizar; te entrega:

1. glyph id desde Unicode,
2. métricas horizontales,
3. puntos TrueType simples,
4. componentes compound con matriz fixed 16.16.

Con eso puedes conectar un flatten/scanline/rasterizador propio después, o un cache estático por glyph si tu engine lo permite.

## Límites honestos

- No interpreta bytecode/hinting TrueType.
- No rasteriza curvas; sólo entrega puntos/contornos.
- No decodifica charstrings CFF/CFF2 todavía.
- No normaliza strings `name` a UTF-8; entrega bytes raw.
- `sfnt_validate_font_checksum()` valida el buffer desde `base_offset` hasta el final del buffer; en TTC conviene validar tablas individuales o pasar un slice exacto si quieres checksum global por face.
