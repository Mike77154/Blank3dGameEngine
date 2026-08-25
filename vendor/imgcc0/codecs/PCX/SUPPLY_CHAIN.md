# Supply Chain

This release line adds a deterministic release pipeline that emits:

- source and binary archives
- CycloneDX SBOMs
- SHA-256 and SHA-512 checksum manifests
- a machine-readable `dist/manifest.json`

## Local commands

```bash
make release-package
make verify-release
SOURCE_DATE_EPOCH=1735689600 make verify-repro
```

## GitHub integration

The repository now ships:

- `.github/workflows/release.yml` for provenance + SBOM attestations
- `.github/workflows/codeql.yml` for semantic static analysis
- `.github/workflows/scorecard.yml` for repository security posture checks
- `.github/dependabot.yml` for GitHub Actions update automation

These workflows are intended to complement the existing build/test/fuzzing posture with release-trust signals and supply-chain metadata.
