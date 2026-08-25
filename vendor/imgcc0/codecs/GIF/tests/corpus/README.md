# GIFF corpus fixtures

Valid-ish / malformed samples shipped with v1.1:

- `bad-signature.gif` — invalid GIF signature
- `comment.gif` — valid comment extension sample
- `truncated-comment.gif` — truncated after extension payload
- `truncated-local-table.gif` — truncated local color table
- `bad-min-code-size.gif` — image data starts with invalid LZW minimum code size (`9`)
- `missing-image-terminator.gif` — raster data misses the terminating zero-length sub-block
- `truncated-lzw-subblock.gif` — raster sub-block advertises more bytes than are present
- `truncated-application.gif` — application extension cut mid-payload
- `bad-local-color-table-short.gif` — local color table flag set without enough bytes following

- `global-loader.gif` — valid “table loader” stream with GCT + trailer only
- `double-gce.gif` — valid-ish stream with duplicated GCE blocks before one image
- `netscape-comment-weird.gif` — loop extension + comment + GCE before image
- `zero-delay-loop.gif` — two-frame looped animation with zero delays

- `empty-comment.gif` — valid empty comment extension before image data
- `interlaced-local.gif` — valid interlaced image carrying a local color table
- `app-comment-empty.gif` — valid loop extension followed by an empty comment and one image

- gce-comment-scope.gif: tiny valid-but-weird fixture for extension-scope and repeated application tests
- double-app-loop.gif: tiny valid-but-weird fixture for extension-scope and repeated application tests