# BMP security notes

This build adds process-wide parser/decode limits so oversized or suspicious BMP objects can be rejected before large allocations happen. By default the library caps input size, width, height, total pixels, decoded RGBA size, palette size, embedded JPEG/PNG payload size and ICC profile size. Set a field to `0` to disable that individual limit, or call `bmp_set_global_limits(NULL)` to restore defaults.

Thread-safety note: limits are process-wide and not synchronized internally. Configure them once during process startup or protect updates externally if multiple threads may parse/decode concurrently.

Operational guidance:

- use the limit API for any untrusted input path
- keep the fuzz targets in CI
- treat symbolic error names and human descriptions as logging aids, not as a stable wire format
