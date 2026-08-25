# Animación WebP en este árbol

## Modelo

- El contenedor se parsea con `GWPDemuxParse()`.
- Los frames `ANMF` exponen:
  - rectángulo (`x`, `y`, `width`, `height`)
  - `duration_ms`
  - `blend_method`
  - `dispose_method`
  - subchunks locales `ALPH` y `VP8 `/`VP8L`
- `GWPAnimDecoder` mantiene un canvas RGBA interno y un buffer temporal para el frame actual.

## Flujo de composición

1. aplicar el `dispose_method` pendiente del frame previo
2. decodificar el bitstream del frame actual a RGBA temporal
3. si hay `ALPH`, aplicar alpha al frame temporal
4. componer sobre el canvas usando blend u overwrite
5. convertir a `RGBA/BGRA/ARGB` al volcar al usuario

## API mínima

```c
GWPAnimDecoderOptions opt;
GWPAnimDecoder dec;
GWPAnimInfo info;

GWPAnimDecoderOptionsInit(&opt);
opt.pixel_format = GWP_PIXFMT_RGBA;
opt.scratch = scratch_mem;
opt.scratch_size = scratch_bytes;

GWPAnimDecoderInit(&dec, webp_data, webp_size, &opt);
GWPAnimDecoderGetInfo(&dec, &info);
while (GWPAnimDecoderHasMoreFrames(&dec)) {
  GWPAnimDecoderGetNext(&dec, rgba_out, rgba_out_size, stride, &timestamp_ms);
}
```

## Ejemplo CLI

```sh
./examples/gwpanimdump anim.webp out/frame
# genera out/frame_000.pam, out/frame_001.pam, ... y out/frame.jsonl
```

## Nota phase 9

El árbol ahora también trae `src/webp/anim_encode.h` y el ejemplo `gwpanimux` para ensamblar animaciones desde frames still WebP ya codificados.

