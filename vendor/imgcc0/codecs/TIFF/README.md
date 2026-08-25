# tifx

`tifx` es una librería TIFF chica, **CC0**, en **C89**, sin `malloc`, y con **fixed-point 16.16** para los tags racionales de resolución.

Esta rama ya trae soporte real para **TIFF clásico** y **BigTIFF**, con parser y writer separados por formato, y además suma **multi-IFD / multipágina en BigTIFF** usando la cadena de `next IFD`, más **SubIFDs** para reduced-resolution o capas relacionadas en variante **tree-style** y **Adobe-style chain opcional**.

---

## Qué sí hace

### Decode / parse

- **TIFF clásico** (`version = 42`) y **BigTIFF** (`version = 43`)
- `II` y `MM` en parse
- imágenes en **strips** y **tiles**
- compresión:
  - `1` = sin compresión
  - `2` = CCITT Group 3 1D / Modified Huffman
  - `3` = CCITT T.4 / Group 3 fax (`1D` y mixed `1D/2D`), incluyendo la extensión opcional de modo sin compresión
  - `4` = CCITT T.6 / Group 4 / MMR, incluyendo la extensión opcional de modo sin compresión
  - `32773` = PackBits
  - `5` = LZW en **strips y tiles**
  - `8` = Deflate / zlib en **strips y tiles**
  - `32946` = Deflate / ZIP legacy en **strips y tiles** (mismo bitstream zlib)
- el decoder zlib/Deflate tolera cierres válidos con **bloque final vacío** antes del Adler-32, además del caso de bloque final con datos
- `Predictor = 2` (**horizontal differencing**) en decode para **gray8**, **RGB24** y **RGBA32** de 8 bits por muestra cuando se usa `LZW` o `Deflate`
- formatos de píxel soportados:
  - bi-level `1 bpp` -> salida **gray8**
  - grayscale `4/8 bpp` -> salida **gray8**
  - palette-color `4/8 bpp` -> salida **RGB24**
  - RGB `8,8,8` chunky o `PlanarConfiguration = 2` -> salida **RGB24**
  - RGBA `8,8,8,8` con `ExtraSamples=1/2` y `PlanarConfiguration = 1/2` -> salida **RGBA32**
- `FillOrder = 1` y `2` para paths CCITT de 1 bit
- en `tifx_image_info` expone:
  - `container_format`
  - `first_ifd_offset`
  - `current_ifd_offset`
  - `next_ifd_offset`
  - `parent_ifd_offset`
  - `page_index`
  - `page_count`
  - `subifd_depth`, `subifd_index`, `subifd_count`
  - `new_subfile_type`
  - `page_number[2]`
  - `t4_options` y `t6_options`
- parse de **page 0** por default vía `tifx_parse_memory()`
- parse de **cualquier página TIFF clásico** vía `tifx_parse_classic_page_memory()`
- parse de **cualquier nodo TIFF clásico** por path de SubIFDs vía `tifx_parse_classic_node_memory()`
- conteo de páginas TIFF clásico vía `tifx_classic_page_count_memory()`
- conteo de hijos SubIFD clásicos vía `tifx_classic_subifd_count_memory()`
- parse de **cualquier página BigTIFF** vía `tifx_parse_bigtiff_page_memory()`
- parse de **cualquier nodo BigTIFF** por path de SubIFDs vía `tifx_parse_bigtiff_node_memory()`
- conteo de páginas BigTIFF vía `tifx_bigtiff_page_count_memory()`
- conteo de hijos SubIFD vía `tifx_bigtiff_subifd_count_memory()`

### Encode

- writer **TIFF clásico** little-endian
- writer **BigTIFF** little-endian
- entrada soportada:
  - **gray8**
  - **RGB24**
  - **RGBA32**
  - **bilevel 1 bpp** en bits empaquetados MSB-first por byte
- compresión de salida:
  - `1` = sin compresión para **gray8**, **RGB24**, **RGBA32** y **bilevel**
  - `2` = CCITT **Modified Huffman** para **bilevel**
  - `3` = CCITT **T.4** para **bilevel**
  - `4` = CCITT **T.6 / Group 4** para **bilevel**
  - `5` = **LZW** para **gray8**, **RGB24**, **RGBA32** y **bilevel** en **strips**, y también para **gray8/RGB24/RGBA32** en **tiles**
  - `8` = **Deflate / zlib** para **gray8**, **RGB24**, **RGBA32** y **bilevel** en **strips**, y también para **gray8/RGB24/RGBA32** en **tiles**
  - `32946` = **Deflate / ZIP legacy** para **gray8**, **RGB24**, **RGBA32** y **bilevel** en **strips**, y también para **gray8/RGB24/RGBA32** en **tiles**
- **multi-strip** configurable con `rows_per_strip`
- **tiles** en el writer clásico de una sola imagen
- **tiled pyramids** en los writers de árbol clásico y BigTIFF (`tile_width` / `tile_length`), incluyendo `Predictor` + `Deflate fixed/dynamic` para `gray8/RGB24/RGBA32`
- `32773` = **PackBits** también en tiles de salida para **gray8**, **RGB24**, **RGBA32** y **bilevel**
- `Predictor = 2` (**horizontal differencing**) configurable en writer para **gray8**, **RGB24** y **RGBA32** con `Compression = 5 / 8 / 32946`
- `deflate_mode` configurable en writer para `Compression = 8 / 32946` con perfiles `auto`, `stored`, `fixed` y `dynamic`
- en `deflate_mode = TIFX_DEFLATE_AUTO`, el encoder hace selección **por bloque** entre `stored` / `fixed` / `dynamic`, y además ajusta `level` / `strategy` / `deflateTune()` sin heap
- `ExtraSamples` en writer para **RGBA32** con `alpha_mode = 1` (associated/premultiplied) o `2` (unassociated)
- `PlanarConfiguration` configurable en writer para **RGB24** y **RGBA32** (`1 = chunky`, `2 = separate planes`) manteniendo la API de entrada/salida en buffer chunky
- `PhotometricInterpretation` configurable para **bilevel** (`0` o `1`)
- `FillOrder` configurable para **bilevel** (`1` o `2`)
- `XResolution` y `YResolution` escritos desde fixed-point 16.16
- en BigTIFF, `StripOffsets` y `StripByteCounts` se escriben como `LONG8`
- writer **multipágina BigTIFF** con:
  - `tifx_write_bigtiff_pages_buffer_size()`
  - `tifx_write_bigtiff_pages_memory()`
- writer **SubIFD tree / Adobe-chain TIFF clásico** con:
  - `tifx_write_classic_tree_buffer_size()`
  - `tifx_write_classic_tree_memory()`
- writer **SubIFD tree / Adobe-chain BigTIFF** con:
  - `tifx_write_bigtiff_tree_buffer_size()`
  - `tifx_write_bigtiff_tree_memory()`
- el writer multipágina mete por página:
  - `NewSubfileType = 2`
  - `PageNumber = {indice, total}`
  - encadenado real por `next IFD`
- el writer puede dejar los hijos fuera de la cadena principal y referenciarlos por `SubIFDs` como **array/tree-style** o como **Adobe-style chain** opcional

---

## Selección de formato

### Parse automático

```c
rc = tifx_parse_memory(&info, file_bytes, file_size);
```

`tifx_parse_memory()` detecta automáticamente si el archivo es TIFF clásico o BigTIFF.

### Writer clásico

```c
tifx_write_params_init(&params);
params.container_format = TIFX_CONTAINER_CLASSIC;
```

### Writer BigTIFF

```c
tifx_write_params_init(&params);
params.container_format = TIFX_CONTAINER_BIGTIFF;
```

Si no pones nada, el writer usa **TIFF clásico**.

### Elegir estilo de `SubIFDs`

```c
tifx_write_params_init(&params);
params.subifd_style = TIFX_SUBIFD_STYLE_TREE;        /* default */
/* o */
params.subifd_style = TIFX_SUBIFD_STYLE_ADOBE_CHAIN;
```

En `TREE`, el tag `SubIFDs` contiene un array de offsets. En `ADOBE_CHAIN`, el padre guarda solo el primer hijo y los hermanos se enlazan por `NextIFD`.

### Escribir LZW / Deflate con `Predictor` y elegir modo Deflate

```c
tifx_write_params_init(&params);
params.pixel_format = TIFX_PIXEL_RGB24;
params.compression = 8U;      /* o 5U / 32946U */
params.predictor = TIFX_PREDICTOR_HORIZONTAL;
params.deflate_mode = TIFX_DEFLATE_DYNAMIC;   /* o TIFX_DEFLATE_AUTO para heurística por bloque */
params.width = width;
params.height = height;
params.stride = width * 3UL;
params.rows_per_strip = 32UL;
params.pixels = rgb_pixels;
```

`5` escribe TIFF **LZW**. `8` escribe **Deflate con zlib**. `32946` acepta el código ZIP/Deflate legacy, aunque para intercambio nuevo conviene preferir `8`. Para `Compression = 8 / 32946`, `deflate_mode` puede ser `TIFX_DEFLATE_AUTO`, `TIFX_DEFLATE_STORED`, `TIFX_DEFLATE_FIXED` o `TIFX_DEFLATE_DYNAMIC`. En `AUTO`, la rama usa heurísticas por bloque y `deflateParams()` / `deflateTune()` para cambiar de perfil dentro del mismo stream zlib cuando conviene.

### Escribir LZW / Deflate en tiles

```c
tifx_write_params_init(&params);
params.pixel_format = TIFX_PIXEL_RGB24;
params.compression = 8U;
params.predictor = TIFX_PREDICTOR_HORIZONTAL;
params.deflate_mode = TIFX_DEFLATE_DYNAMIC;
params.tile_width = 64UL;
params.tile_length = 64UL;
params.width = width;
params.height = height;
params.stride = width * 3UL;
params.pixels = rgb_pixels;
```

En tiles, los paths `LZW` y `Deflate` de esta rama quedan habilitados para **gray8**, **RGB24** y **RGBA32**. Para **bilevel**, los tiles de salida siguen limitados a **sin compresión** o **PackBits**.

---

## API extra de multipágina y SubIFDs

```c
int tifx_parse_classic_page_memory(...);
int tifx_parse_classic_node_memory(...);
int tifx_classic_page_count_memory(...);
int tifx_classic_subifd_count_memory(...);

int tifx_parse_bigtiff_page_memory(...);
int tifx_parse_bigtiff_node_memory(...);
int tifx_bigtiff_page_count_memory(...);
int tifx_bigtiff_subifd_count_memory(...);

unsigned long tifx_write_classic_tree_buffer_size(...);
int tifx_write_classic_tree_memory(...);

unsigned long tifx_write_bigtiff_pages_buffer_size(...);
int tifx_write_bigtiff_pages_memory(...);

unsigned long tifx_write_bigtiff_tree_buffer_size(...);
int tifx_write_bigtiff_tree_memory(...);
```

### Escribir 3 páginas BigTIFF

```c
tifx_write_params pages[3];
unsigned long written;
int rc;

/* llenar pages[0], pages[1], pages[2] */

rc = tifx_write_bigtiff_pages_memory(file_buffer,
                                     file_buffer_size,
                                     pages,
                                     3UL,
                                     &written);
```

### Leer la página 2 de un BigTIFF multipágina

```c
tifx_image_info info;
int rc;

rc = tifx_parse_bigtiff_page_memory(&info,
                                    file_bytes,
                                    file_size,
                                    2UL);
```

---

## Nota de portabilidad BigTIFF

Esta implementación BigTIFF usa `unsigned long` como entero base para offsets/cuentas internas.

Eso significa:

- en targets LP64 normales (Linux/macOS 64-bit), el path BigTIFF funciona bien
- si `unsigned long` es de 32 bits, `tifx_bigtiff_available()` devuelve `0`
- en ese caso, las funciones BigTIFF regresan `TIFX_ERR_UNSUPPORTED`

La rama sigue siendo **C89** porque no usa `long long`, heap, ni dependencias externas.

---

## Límites deliberados

- multipágina encadenada por `NextIFD` sigue implementada en **BigTIFF**
- `SubIFDs` en árbol y **Adobe-style chain opcional** ya están implementados en **TIFF clásico** y **BigTIFF**
- el parser expone navegación por path explícito y, cuando detecta `SubIFDs` con un solo offset, también sigue la cadena `NextIFD` entre hermanos para cubrir la variante Adobe-style
- no tiles CCITT/JPEG en writer
- los tiles escritos hoy aceptan **sin compresión**, **PackBits**, **LZW** y **Deflate** para **gray8/RGB24/RGBA32**; **bilevel** en tiles sigue limitado a **sin compresión** o **PackBits**
- `Predictor` horizontal queda validado para `LZW` / `Deflate` cuando aplica a datos de 8 bits por muestra
- no JPEG
- no `PlanarConfiguration = 2` para gray/palette/bilevel
- no orientaciones distintas de `1` al decodificar
- no CMYK / YCbCr / Lab / float
- `RGBA32` sigue limitado a un único `ExtraSamples` alpha
- no heap (`malloc`, `calloc`, `realloc`, `free`)
- writer BigTIFF solo **little-endian**
- el writer BigTIFF de una sola imagen y el multipágina por `NextIFD` siguen strip-based; los **tiles BigTIFF** hoy viven en el writer de árbol / pyramids
- para interoperabilidad, el writer sigue escribiendo `ImageWidth`, `ImageLength` y `RowsPerStrip` como `LONG` de 32 bits
- no genera la extensión opcional de modo sin compresión dentro de T.4 / T.6 al escribir
- para T.6 el writer solo acepta `t6_options = 0`
- para T.4 el writer acepta solo los bits `0` y `2` de `t4_options`
- el multipágina BigTIFF queda acotado por `TIFX_MAX_PAGES`

---

## Estructura del proyecto

```text
tifx_cc0_predictor_tiles_deflate_huff/
├── include/
│   └── tifx.h
├── src/
│   ├── tifx.c
│   ├── tifx_dispatch.c
│   ├── tifx_bigtiff_parse.c
│   ├── tifx_bigtiff_write.c
│   └── tifx_internal.h
├── docs/
│   ├── BIGTIFF_BRANCH_NOTES.md
│   ├── BIGTIFF_MULTIPAGE_NOTES.md
│   ├── BIGTIFF_SUBIFD_TREE_NOTES.md
│   ├── CLASSIC_SUBIFD_TILED_PYRAMIDS.md
│   ├── PACKBITS_ADOBE_SUBIFD_NOTES.md
│   ├── PLANARCONFIG2_NOTES.md
│   ├── RGBA_ALPHA_NOTES.md
│   ├── LZW_DEFLATE_NOTES.md
│   └── PREDICTOR_TILED_DEFLATE_NOTES.md
├── examples/
│   ├── write_compressed.c
│   ├── inspect_tiff.c
│   ├── write_bilevel.c
│   ├── write_gradient.c
│   ├── write_multipage.c
│   └── write_subifd_tree.c
├── tests/
│   ├── data/
│   │   ├── g3_1d_fill.tif
│   │   ├── g3_1d_fill1.tif
│   │   ├── g3_1d_uncompressed.tif
│   │   ├── g3_2d_fill.tif
│   │   ├── g3_2d_uncompressed.tif
│   │   ├── g4_fill1.tif
│   │   ├── g4_fill2.tif
│   │   └── g4_uncompressed.tif
│   └── test_tifx.c
├── LICENSE-CC0.txt
├── Makefile
└── README.md
```

---

## Build

```bash
make
```

### Ejecutar tests

```bash
make test
```

### Generar ejemplos

```bash
./examples/write_gradient
./examples/write_bilevel
./examples/write_multipage
```

### Inspeccionar una página específica de un BigTIFF

```bash
./examples/inspect_tiff multipage_big.tif 2 page2.ppm
```
