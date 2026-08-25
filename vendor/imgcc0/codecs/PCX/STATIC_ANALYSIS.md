# PCX static analysis (1.16.0)

Static-analysis smoke now covers:

- GCC `-fanalyzer` on representative translation units
- Clang Static Analyzer via `clang --analyze`
- header compilation under the supported language standards for consumers

The Makefile exposes `analyzer-smoke`, `header-matrix-smoke`, and `compiler-contracts-audit` to keep these checks reproducible in CI and local development.
