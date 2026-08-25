# Consumer verification

Offline verification:

```bash
make release-package
make verify-release
make verify-provenance
```

Manual verification:

```bash
python3 scripts/verify_release_artifacts.py --dist-dir dist
python3 scripts/verify_provenance.py --dist-dir dist --policy dist/verification-policy.json
```

For GitHub-hosted releases, consumers can additionally verify:

- immutable releases with `gh release verify TAG`
- a local asset against a published release with `gh release verify-asset TAG PATH`
- artifact attestations with `gh attestation verify`

The local detached signatures are meant to support offline / air-gapped validation of the bundle contents, while GitHub artifact attestations provide a higher-level provenance path for public release consumers.
