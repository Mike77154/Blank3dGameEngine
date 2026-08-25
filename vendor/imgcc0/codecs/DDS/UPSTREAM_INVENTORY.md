# Upstream support inventory

The supplied `DDS.zip` contained 31 files. The restored sanitized package contains every one of those paths.

The supplied archive did **not** contain dedicated `corpus/`, `fuzz/`, `.github/`, CI, or `docs/` directories. Its documentation was the top-level `README.md`; that full documentation has been restored and updated for the C89/static-storage contract.

The sanitization work itself created a deterministic 36-artifact byte-parity corpus outside the first delivered ZIP. This revision packages that corpus under `corpus/parity_expected/` together with `tools/parity_harness.c` and `tools/verify_parity.sh`.

Additional regression/support files retained from the sanitized edition are `SANITIZATION_REPORT.md`, `PARITY_SHA256.txt`, `tests/golden_vectors.h`, `tests/test_byte_vectors.c`, `tests/test_features.c`, and `tools/audit_c89.sh`.
