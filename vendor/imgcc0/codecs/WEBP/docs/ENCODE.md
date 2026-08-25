# Encode API (phase10)

## Still

```c
GWPEncodeConfig cfg;
GWPEncodeConfigInit(&cfg);
cfg.output_buffer = out_buf;
cfg.output_buffer_size = out_cap;

status = GWPEncodePixels(rgba,
                         width,
                         height,
                         stride,
                         GWP_RAW_RGBA,
                         &cfg,
                         &out_size);
```

## Animated raw frames

```c
GWPAnimEncoderOptions aopt;
GWPAnimEncoder enc;
GWPAnimFrameSpec spec;

GWPAnimEncoderOptionsInit(&aopt);
aopt.work_mem = work_mem;
aopt.work_mem_size = work_mem_size;
aopt.minimize_size = GWP_TRUE;

auto_init...

GWPAnimEncoderInit(&enc, canvas_w, canvas_h, &aopt);
GWPAnimFrameSpecInit(&spec);

GWPAnimEncoderAddFramePixels(&enc,
                             rgba_frame,
                             canvas_w * 4,
                             GWP_RAW_RGBA,
                             &spec,
                             &cfg);

GWPAnimEncoderAssemble(&enc, out_webp, out_cap, &out_size);
```

## Nota honesta

La ruta cruda de esta fase es **lossless-first**. Los knobs de config quedan sembrados para crecer, pero hoy el encode completo implementado es VP8L literal-only.
