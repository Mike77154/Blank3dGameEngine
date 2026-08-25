# Environment gates

The release workflow now references the **`release`** GitHub Actions environment.

## Why

The environment indirection keeps signing material and any future deployment credentials unavailable until the release job is explicitly approved in GitHub.

## Configure in GitHub

Recommended settings for the `release` environment:

- required reviewers enabled
- only protected branches/tags allowed
- release secrets scoped only to this environment
- optional wait timer if you want a manual hold before publication

The intended state is documented in `governance/environments/release.json`.
