# Classic TIFF SubIFDs + tiled pyramids

Esta rama agrega dos piezas nuevas sobre el tronco anterior:

- `SubIFDs` en **TIFF clásico** usando el tag `330` con offsets `LONG`
- pirámides **tiled** en writers de árbol para **TIFF clásico** y **BigTIFF**

## Qué se implementó

### Parse / navegación

- `tifx_parse_classic_page_memory(...)`
- `tifx_parse_classic_node_memory(...)`
- `tifx_classic_page_count_memory(...)`
- `tifx_classic_subifd_count_memory(...)`

El parser clásico ya puede:

- contar páginas top-level por cadena `NextIFD`
- navegar hijos `SubIFD` por path explícito
- exponer offsets actuales/padre/siguiente en `tifx_image_info`
- leer imágenes `strip-based` o `tile-based`

### Writer de árbol clásico

- `tifx_write_classic_tree_buffer_size(...)`
- `tifx_write_classic_tree_memory(...)`

El layout que emite es conservador:

- páginas top-level encadenadas por `NextIFD`
- hijos fuera de la cadena principal
- `SubIFDs` como array de offsets `LONG`
- `NewSubfileType` respetado por nodo
- `PageNumber` autogenerado en páginas top-level cuando hay más de una

### Tiled pyramids

El writer de árbol clásico y el de árbol BigTIFF aceptan:

- `tile_width`
- `tile_length`

cuando ambos son no-cero, el nodo se escribe como imagen tiled.

## Restricciones deliberadas

- el writer tiled actual escribe **solo `Compression=1`**
- `tile_width` y `tile_length` deben ser múltiplos de `16`
- el decoder tiled actual soporta `Compression=1` y `32773`
- el writer BigTIFF single-image / multipage por `NextIFD` sigue orientado a strips

## Ejemplo rápido

```c
tifx_tiff_node pages[1];
tifx_tiff_node children[1];

tifx_write_params_init(&children[0].image);
children[0].image.container_format = TIFX_CONTAINER_CLASSIC;
children[0].image.pixel_format = TIFX_PIXEL_GRAY8;
children[0].image.width = 128UL;
children[0].image.height = 128UL;
children[0].image.stride = 128UL;
children[0].image.tile_width = 16UL;
children[0].image.tile_length = 16UL;
children[0].image.new_subfile_type = 1UL;
children[0].image.pixels = level1_pixels;
children[0].children = 0;
children[0].child_count = 0UL;

tifx_write_params_init(&pages[0].image);
pages[0].image.container_format = TIFX_CONTAINER_CLASSIC;
pages[0].image.pixel_format = TIFX_PIXEL_GRAY8;
pages[0].image.width = 256UL;
pages[0].image.height = 256UL;
pages[0].image.stride = 256UL;
pages[0].image.tile_width = 16UL;
pages[0].image.tile_length = 16UL;
pages[0].image.pixels = level0_pixels;
pages[0].children = children;
pages[0].child_count = 1UL;
```

Luego:

```c
unsigned long written;
int rc;

rc = tifx_write_classic_tree_memory(file_buf,
                                    file_buf_size,
                                    pages,
                                    1UL,
                                    &written);
```
