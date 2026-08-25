# PCX 1.5.0

This round focuses on release trust and software-supply-chain maturity.

## Added

- deterministic source and binary release packaging
- CycloneDX 1.7 SBOM emission for source and staged binary layouts
- SHA-256 / SHA-512 checksum manifests
- `dist/manifest.json` with sizes and digests
- reproducibility verification tooling
- release provenance and SBOM attestation workflow
- CodeQL + OpenSSF Scorecard + Dependabot workflows

## Local smoke commands

```bash
make release-package
make verify-release
SOURCE_DATE_EPOCH=1735689600 make verify-repro
```
