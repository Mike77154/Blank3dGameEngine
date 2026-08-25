# tests

These are tiny synthetic files for parser and decoder smoke tests. They are not real production fonts.

```sh
make
./eottool info tests/minimal_raw_v1.eot
./eottool extract tests/minimal_raw_v1.eot tests/minimal_raw_v1.out.ttf
./eottool info tests/synthetic_mtx.eot
./eottool mtx-unpack tests/synthetic_mtx.eot tests/synth
```

Expected MTX synthetic outputs:

```text
tests/synth.block1_font_tables.ctf -> "CTF1 hello font tables"
tests/synth.block2_push_data.ctf   -> "pushdata"
tests/synth.block3_glyph_insns.ctf -> "glyphinsns"
```
