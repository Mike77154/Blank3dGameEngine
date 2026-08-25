# imgcc0 0.6.0 PSD89 + ZRAGF decode integration

The supplied PSD89 replacement vendor remains byte-identical to its uploaded package. The imgcc0 facade calls `psd89_read()` and `psd89_decode_composite_u8()` and converts the supported Gray/RGB 8-bpc composite output to caller-owned RGBA8.

In 0.6.0 the PSD source still calls the zlib-shaped API internally, but the strict build places `compat/` first in the include path. `compat/zlib.h` maps those calls to Protocol89 ZRAGF, so PSD ZIP and ZIP+prediction decode no longer require system zlib in the top-level image-decoder assembly.

Validation: 24/24 supplied PSD corpus/example files produce byte-identical canonical RGBA output between the direct PSD vendor oracle and the imgcc0 facade using ZRAGF. RAW, RLE, ZIP, ZIP+prediction, groups, user masks, vector masks and clipping examples are represented in that set. The strict source audit remains clean and no system zlib symbols remain unresolved in `libimgcc0.a`.

The PSD vendor itself is not modified; the backend substitution happens at the imgcc0 compatibility layer.
