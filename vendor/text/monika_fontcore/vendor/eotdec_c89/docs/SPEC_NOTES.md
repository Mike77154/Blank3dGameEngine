# EOT notes used by this implementation

- EOT is a single `EMBEDDEDFONT` structure.
- EOT wrapper fields are little-endian; embedded OpenType/TrueType `FontData` remains big-endian SFNT.
- Supported EOT header versions: `0x00010000`, `0x00020001`, `0x00020002`.
- `MagicNumber` must be `0x504C`.
- Important flags:
  - `TTEMBED_SUBSET = 0x00000001`
  - `TTEMBED_TTCOMPRESSED = 0x00000004`
  - `TTEMBED_EMBEDEUDC = 0x00000020`
  - `TTEMBED_WEBOBJECT = 0x00000080`
  - `TTEMBED_XORENCRYPTDATA = 0x10000000`
- RootString checksum for v2.2 is `sum(root bytes) ^ 0x50475342`.
- XOR encryption/decryption is `byte ^= 0x50`.
- If both compression and XOR are present, EOT must be XOR-decoded first, then decompressed.

This package implements raw and XOR-only extraction. MTX compression is detected and rejected cleanly.

## MTX update

`TTEMBED_TTCOMPRESSED` means the EOT `FontData` contains MicroType Express data instead of a directly usable SFNT. The decoder now exposes `mtx-info` and `mtx-unpack` to parse and LZCOMP-decompress that MTX payload into its three CTF streams. The final CTF-to-TTF reconstruction remains a separate stage.
