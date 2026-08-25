# LZW / Deflate notes

Esta vuelta mete dos paths nuevos sin heap:

- `Compression = 5` -> **TIFF LZW**
- `Compression = 8` -> **Adobe Deflate / zlib**
- `Compression = 32946` -> **ZIP/Deflate legacy** compatible en bitstream con `8`

## Alcance práctico de esta rama

### LZW

- encoder y decoder **sin `malloc`**
- tabla fija de 12 bits
- `ClearCode = 256`
- `EOI = 257`
- cada **strip** arranca con `ClearCode` y termina con `EOI`
- códigos empacados **high-to-low** dentro del stream
- `FillOrder = 1` solamente
- writer y tests cubren tamaños que ya fuerzan cambios de 9 -> 10 -> 11 bits

### Deflate

- stream **zlib completo** por strip
- header zlib + bloques DEFLATE + trailer Adler-32
- implementación **sin heap** usando `zlib` enlazado con un **allocator fijo** (sin `malloc`) y soporte de bloques **stored**, **fixed Huffman** y **dynamic Huffman**
- no usa diccionario preset
- `Compression = 8` y `Compression = 32946` comparten el mismo payload
- para intercambio nuevo conviene preferir `8`

## Qué sí hace ya en esta rama

- `Predictor = 2` para **gray8**, **RGB24** y **RGBA32** con `LZW` / `Deflate`
- `LZW` / `Deflate` en **tiles** para **gray8**, **RGB24** y **RGBA32**
- `deflate_mode` para elegir entre **stored**, **fixed Huffman** y **dynamic Huffman**
- decoder zlib/Deflate que acepta cierres válidos con bloque final vacío antes del Adler-32

## Qué no hace todavía

- no compresión JPEG
- no `Predictor` para **bilevel**
- no `LZW` / `Deflate` tiled para **bilevel**

## Ejemplos

```bash
make
./examples/write_compressed
./examples/inspect_tiff gradient_gray_lzw.tif
./examples/inspect_tiff gradient_rgb_deflate.tif
```

## Intención de interoperabilidad

- LZW sigue la convención TIFF clásica por strip
- Deflate escribe un stream zlib completo por strip
- el writer acepta `32946` por compatibilidad, pero la salida recomendada para archivos nuevos es `8`
