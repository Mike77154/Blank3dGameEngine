# Rulesets as code

This release adds **importable GitHub rulesets** under `.github/rulesets/` and keeps them in sync from a single source of truth in `governance/required_status_checks.json`.

## Files

- `.github/rulesets/default-branch-protection.json`
- `.github/rulesets/release-tags-protection.json`
- `governance/required_status_checks.json`
- `governance/repository_desired_state.json`
- `scripts/render_rulesets.py`
- `scripts/audit_repo_governance.py`
- `scripts/gh_apply_governance.py`

## Recommended flow

1. Edit `governance/required_status_checks.json` if the stable required checks change.
2. Run `make ruleset-render governance-audit`.
3. Review the generated JSON files.
4. Apply them with `python3 scripts/gh_apply_governance.py --owner ORG --repo REPO --dry-run` or with a real token.

The default branch ruleset is designed to enforce pull requests, signed commits, linear history, required reviews, required status checks, and merge queue semantics. The tag ruleset protects `v*` tags against mutation after creation.
