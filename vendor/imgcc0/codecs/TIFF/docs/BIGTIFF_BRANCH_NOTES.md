# BigTIFF branch notes

Esta rama ya implementa **BigTIFF real** en paralelo al path clásico.

## Separación interna

```text
src/
├── tifx.c               -> core clásico + decoder + helpers CCITT
├── tifx_dispatch.c      -> wrapper público auto-detect / auto-dispatch
├── tifx_bigtiff_parse.c -> parser BigTIFF
├── tifx_bigtiff_write.c -> writer BigTIFF
└── tifx_internal.h      -> helpers internos compartidos con el writer BigTIFF
```

## Decisiones de diseño

- `tifx_parse_memory()` detecta `42` vs `43` y despacha solo al parser correcto
- `tifx_write_memory()` despacha según `params.container_format`
- el decoder sigue siendo compartido: una vez llenado `tifx_image_info`, el path de decode no necesita distinguir entre TIFF clásico y BigTIFF
- el writer BigTIFF usa:
  - header de 16 bytes
  - contador de tags de 64 bits
  - entradas IFD de 20 bytes
  - offsets/cuentas de strips como `LONG8`
  - alineación a 8 bytes para strips, IFD y arrays auxiliares

## Limitaciones conscientes de esta rama

- writer BigTIFF solo little-endian
- `ImageWidth`, `ImageLength` y `RowsPerStrip` se mantienen como `LONG` de 32 bits para no forzar tipos exóticos en tags que casi siempre caben ahí
- el soporte BigTIFF depende de que `unsigned long` sea de 64 bits
- no hay múltiples IFDs ni tiles

## Tests añadidos

- roundtrip **gray8 BigTIFF**
- roundtrip **RGB24 BigTIFF**
- roundtrip **bilevel T.6 BigTIFF multi-strip**

La suite completa sigue corriendo con:

```bash
make test
```
