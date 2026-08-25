# RenList89 API

`rl89_parse()` fills a caller-owned `RenList89`. On syntax failure,
`error_line` is 1-based and `last_error` contains the reason category.

`rl89_property()` gives consumers recipe metadata without teaching RenList89
what the metadata means. This is the main agnostic boundary.
