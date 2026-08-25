# BMP 1.8.0 provenance and detached signatures

This release train adds two verification layers on top of the existing packaging work:

1. **Detached signatures** for `manifest.json` and the provenance statement.
2. A **SLSA-style provenance statement** in in-toto statement form (`*.intoto.jsonl`).

Release bundles now ship:

```text
dist/
  manifest.json
  manifest.json.sig
  SHA256SUMS
  SHA512SUMS
  bmp-1.8.0-provenance.intoto.jsonl
  bmp-1.8.0-provenance.intoto.jsonl.sig
  bmp-1.8.0-release-public.pem
  bmp-1.8.0-signing.json
  verification-policy.json
```

The provenance statement captures:

- subject digests for every shipped artifact
- a stable builder identity
- build type
- expected external parameters (`project`, `version`, `platform`)
- a deterministic invocation id for reproducible local smoke builds

Production intent:

- local smoke builds use a **development fixture key** stored under `tests/fixtures/`
- the **private** fixture key is excluded from source release bundles
- CI / real release automation should pass a separate production private key and continue to publish GitHub artifact attestations
