# Memory and polygon budget

On the intended Win32/MinGW32 ABI (`long` = 4 bytes):

- `km89_vertex`: 12 bytes
- `km89_triangle`: normally 8 bytes including alignment
- maximum public vertex buffer: 49,152 bytes
- maximum public triangle buffer: 65,536 bytes
- conservative caller-side maximum working mesh buffer: about **112 KiB**

The 33 v1.1 presets generate:

- **664–948 vertices**
- **832–1,280 triangles**
- largest current preset data: about **21.2 KiB** on Win32

The public limits remain intentionally generous for custom descriptors and future variants.
A game can reduce the buffers after checking the maximum preset it actually ships.
The generator owns no heap and retains no per-instance state.
