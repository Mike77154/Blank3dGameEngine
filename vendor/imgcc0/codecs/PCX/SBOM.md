# SBOM

The release tooling generates CycloneDX JSON SBOMs for both the source tree and the staged binary
layout.

Artifacts emitted into `dist/`:

- `*-source.cdx.json`
- `*-binary.cdx.json`

Each document records file-level hashes so consumers can cross-check integrity against the release
checksum manifests.

## Commands

```bash
make release-package
make verify-release
```
