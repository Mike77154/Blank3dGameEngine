# PCX workflow security

This release hardens GitHub workflow governance around four ideas:

1. All third-party actions are pinned to full commit SHAs and recorded in `policies/approved_actions.json`.
2. Workflows default to `permissions: {}` and then grant only the scopes each job requires.
3. CI workflows declare `concurrency` and job `timeout-minutes` to reduce accidental overlap and hung runs.
4. `scripts/audit_github_policy.py` enforces the local policy and can emit JSON/Markdown audit reports.

Recommended maintainer flow:

```bash
make policy-audit
git diff .github/workflows policies/approved_actions.json
```

If you update a pinned action, update both the workflow reference and `policies/approved_actions.json`, then rerun the audit.
