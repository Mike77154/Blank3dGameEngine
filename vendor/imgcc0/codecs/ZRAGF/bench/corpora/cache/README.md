# Phase 70 corpus cache

Place externally downloaded corpus files here when running the phase 70 fetcher offline or in restricted environments.

Expected filenames from the default phase 70 manifest:

- `cantrbry.zip`
- `pics-3.8.0.tar.gz`
- `basn0g01.png`
- `basn2c08.png`
- `basn6a16.png`
- `tp1n3p08.png`

Then run:

```bash
python3 tools/fetch_phase70_corpora.py \
  --manifest bench/corpora/phase70_public_manifest.json \
  --root bench/corpora/materialized_phase70 \
  --archive-cache bench/corpora/cache \
  --cache-only \
  --lock-out bench/results/phase70/phase70_corpus_lock.json \
  --freeze-manifest-out bench/results/phase70/phase70_frozen_manifest.json
```
