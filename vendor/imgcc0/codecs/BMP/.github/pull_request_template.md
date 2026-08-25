# Pull request checklist

- [ ] I reviewed any `.github/workflows/*` change for pinned actions, explicit permissions, and timeout/concurrency settings.
- [ ] I updated `policies/approved_actions.json` if I changed any GitHub Action SHA.
- [ ] I ran `make policy-audit` locally or in CI.
- [ ] I evaluated whether the change affects release provenance, SBOMs, or consumer verification docs.
- [ ] I documented any operational or security impact in release notes.
