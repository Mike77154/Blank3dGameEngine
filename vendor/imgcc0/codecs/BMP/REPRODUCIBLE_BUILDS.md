# Reproducible Builds

The release artifacts are normalized around `SOURCE_DATE_EPOCH`.

If `SOURCE_DATE_EPOCH` is set, the release packager uses it as the canonical archive timestamp.
If it is unset, the tooling derives a stable epoch from the source tree so repeated runs over the
same tree still converge on byte-identical outputs.

## Commands

```bash
SOURCE_DATE_EPOCH=1735689600 make release-package
SOURCE_DATE_EPOCH=1735689600 make verify-repro
```

## What is normalized

- tar/gzip timestamps
- zip timestamps
- archive entry ordering
- ownership metadata in tar archives
- manifest ordering
- checksum file ordering
