# External corpus plan

This release wires the PCX library to an external-compatibility workflow based on public PCX regressions and image fixtures.

- `tests/external_corpus/pcx_manifest.json` lists externally curated cases.
- `scripts/fetch_external_corpus.py` hydrates those cases into `tests/external_corpus/downloaded/`.
- `scripts/generate_rare_cases.py` builds a local baseline corpus for offline CI.
- `scripts/run_compat_matrix.py` evaluates generated and hydrated files with `tools/pcx_compat_cli`.
