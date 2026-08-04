# zlib89

A C89, no-heap inflater for RFC 1950 zlib streams and RFC 1951 DEFLATE blocks.

## Properties

- no malloc / realloc / free
- no floats
- caller-owned output buffer
- fixed-size scratch object (`Zlib89Scratch`)
- supports stored, fixed Huffman and dynamic Huffman blocks
- validates CMF/FLG and Adler-32 for zlib streams
- rejects preset-dictionary streams by default

## API

```c
Zlib89Scratch scratch;
zlib89_u32 written;
int rc = zlib89_inflate_zlib(src, src_size,
                             dst, dst_size,
                             0u,
                             &scratch,
                             &written,
                             0);
```

There is also `zlib89_inflate_raw()` for raw DEFLATE payloads.
