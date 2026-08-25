# BigTIFF multipage notes

Esta rama agrega multipágina en BigTIFF sin heap.

## Modelo

- las páginas se representan como una cadena principal de IFDs enlazados por `next IFD`
- cada página puede tener:
  - tamaño distinto
  - formato de píxel distinto
  - compresión distinta dentro del subconjunto ya soportado por `tifx`
- el writer multipágina escribe `NewSubfileType = 2` y `PageNumber = {page_index, page_count}`

## API

```c
int tifx_parse_bigtiff_page_memory(...);
int tifx_bigtiff_page_count_memory(...);

unsigned long tifx_write_bigtiff_pages_buffer_size(...);
int tifx_write_bigtiff_pages_memory(...);
```

## Qué no hace todavía

- `SubIFDs`
- writer multipágina para TIFF clásico
- metadatos de documento más ricos (`PageName`, `DocumentName`, etc.)
- streaming incremental; el writer sigue necesitando un buffer de salida completo ya reservado
