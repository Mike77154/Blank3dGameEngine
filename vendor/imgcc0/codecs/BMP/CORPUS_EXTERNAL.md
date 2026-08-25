# External corpus plan

This release wires the BMP library to an external-compatibility workflow based on public BMP corpora.

- `tests/external_corpus/bmp_manifest.json` lists externally curated cases.
- `scripts/fetch_external_corpus.py` hydrates those cases into `tests/external_corpus/downloaded/`.
- `scripts/generate_rare_cases.py` builds a local baseline corpus for offline CI.
- `scripts/run_compat_matrix.py` evaluates generated and hydrated files with `tools/bmp_compat_cli`.

The manifest intentionally distinguishes:

- `good`: expected parse/decode success
- `questionable`: tolerated or edge-layout files
- `bad`: expected parse rejection
