# Performance and observability

This release adds three report-oriented workflows:

- `make benchmark-report` -> JSON + Markdown benchmark summaries.
- `make coverage-report` -> gcov-backed line coverage summaries.
- `make compat-report` -> generated/external compatibility matrices.

These reports are intentionally file-based so they can be archived as CI artifacts.
