# Integration map

```text
mfc_open_memory()
  detect container
    WOFF1 -> woff1_decode_to_sfnt()
    WOFF2 -> w2f_decode_woff2_to_sfnt()
    EOT   -> scan embedded SFNT bytes
  open SFNT
    sfnt_open()
    if glyf: gwt_font_init()
    if CFF/CFF2/OTTO: otf_parse()

mfc_open_gmyy_spritefont()
  gmyy_decode_sprite_yy()
  gmyy_decode_font_call_gml()
  gmyy_build_spritefont()

mfc_open_mugen_text()
  mft_font_text_parse()

mfc_shape_static_utf8()
  ghb_shape_utf8()
```
