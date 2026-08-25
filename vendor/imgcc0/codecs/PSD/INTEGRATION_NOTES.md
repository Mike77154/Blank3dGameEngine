# INTEGRATION NOTES

## 1. `src/psd89_compose.c`

Replace the old anchor-only vector coverage path with:

1. At layer-state setup time, allocate one `psd89_vector_flatten_result` per active vector-masked layer.
2. Call `psd89_vector_flatten_mask(doc->width, doc->height, &layer->vector_mask, &flat_opt, &flat_result)`.
3. For each pixel center `(x, y)`, use `psd89_vector_coverage_u8(&layer->vector_mask, &flat_result, x, y)`.
4. If `layer->vector_mask_global_present && layer->vector_mask_global != 0`, apply the vector mask in the **final crossfade** stage rather than the shape stage.
5. If `layer->layer_mask_global_present && layer->layer_mask_global != 0`, apply the raster user mask in the same final-crossfade stage.

That maps directly onto the documented meaning of `lmgm` / `vmgm`: the mask is used in a final crossfade masking the layer and effects, instead of shaping the layer/effects earlier.

## 2. `src/psd89_read.c`

For tagged blocks:

- key `lmgm`
- key `vmgm`

the payload is:

- 1 byte boolean
- 3 bytes padding

Use:

- `psd89_mask_global_bool_parse()` to decode the payload into:
  - `layer->layer_mask_global_present`
  - `layer->layer_mask_global`
  - `layer->vector_mask_global_present`
  - `layer->vector_mask_global`

## 3. `src/psd89_write.c`

When the corresponding `*_present` bit is set:

- emit `'8BIM'`
- emit key `lmgm` or `vmgm`
- emit length `4`
- write the 4-byte payload returned by `psd89_mask_global_bool_write()`

## 4. `src/psd89_zip.c`

After inflate/deflate, compute:

- `expected = rows * cols * channels`
- `decoded = actual decoded bytes`

and call:

```c
psd89_zip_diag diag;
psd89_zip_diag_init(&diag);
rc = psd89_zip_diag_check_planar(comp, rows, cols, channels,
                                 compressed_bytes, decoded, zlib_rc, &diag);
```

Bubble `diag.code` or map it into the existing library error vocabulary if you prefer to keep the old public API small.
