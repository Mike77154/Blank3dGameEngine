# BMP 1.7.0

This release adds **offline-verifiable release integrity** on top of the previous packaging work and aligns BMP with the harder supply-chain path already established for PCX.

## New

- detached signatures for `manifest.json` and the provenance statement
- in-toto / SLSA-style provenance generation in `dist/`
- `verification-policy.json` for deterministic policy-based consumer checks
- `make verify-provenance`
- development signing fixtures for reproducible local smoke builds
- updated CI / release smoke to exercise provenance verification

## Notes

- production automation should pass a dedicated signing key
- GitHub artifact attestations remain part of the release story for online consumers
