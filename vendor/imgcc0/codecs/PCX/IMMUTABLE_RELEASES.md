# Immutable releases

This repository now carries governance automation that can enable GitHub **immutable releases** together with importable rulesets for release tags.

## Why both

- the tag ruleset prevents accidental or malicious tag mutation for `v*`
- immutable releases prevent published release assets and their associated tags from being changed after publication

## Applying the policy

Dry-run a plan:

```bash
python3 scripts/gh_apply_governance.py --owner ORG --repo REPO --dry-run --out tests/governance_reports/github_apply_plan.sh
```

Apply for real:

```bash
export GITHUB_TOKEN=...
python3 scripts/gh_apply_governance.py --owner ORG --repo REPO
```

## Recommended maintainer flow

1. Create a draft release.
2. Attach all source/binary/SBOM/provenance assets.
3. Publish the release only when the asset set is complete.
