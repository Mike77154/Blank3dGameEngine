# Anim encode API

## Idea

La API está pensada para mantener el core sin `malloc`:

- el encoder guarda **referencias** a bytes de entrada
- el llamador debe mantener vivos esos buffers hasta `GWPAnimEncoderAssemble()`

## Flujo básico

1. `GWPAnimEncoderInit()`
2. `GWPAnimEncoderSetAnimationParams()`
3. `GWPAnimEncoderAddFrameWebP()` o `GWPAnimEncoderAddFrameBitstream()`
4. `GWPAnimEncoderEstimateSize()`
5. `GWPAnimEncoderAssemble()`

## Example

```c
GWPAnimEncoder enc;
GWPAnimEncoderOptions opts;
GWPMuxAnimParams params;
GWPAnimFrameSpec spec;

GWPAnimEncoderOptionsInit(&opts);
GWPAnimEncoderInit(&enc, 640, 480, &opts);
params.bgcolor = 0x00000000u;
params.loop_count = 0u;
GWPAnimEncoderSetAnimationParams(&enc, &params);

GWPAnimFrameSpecInit(&spec);
spec.duration_ms = 100u;
spec.x_offset = 0u;
spec.y_offset = 0u;
spec.blend_method = (GWPu8)GWP_ANIM_BLEND;
spec.dispose_method = (GWPu8)GWP_ANIM_DISPOSE_NONE;

GWPAnimEncoderAddFrameWebP(&enc, &frame0_webp, &spec);
```

## CLI de ejemplo

`examples/gwpanimux` arma una animación desde un manifest TSV:

```text
path\tduration_ms\tx\ty\tblend\tdispose
```
