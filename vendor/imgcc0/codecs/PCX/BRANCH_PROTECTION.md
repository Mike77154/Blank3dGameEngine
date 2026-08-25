# Branch protection checklist

Recommended GitHub settings for the default branch:

- Require a pull request before merging.
- Require status checks to pass before merging. At minimum: `ci`, `codeql`, `scorecard`, and `dependency-review` (for public repositories).
- Require branches to be up to date before merging.
- Dismiss stale pull request approvals when new commits are pushed.
- Require review from code owners after replacing `CODEOWNERS.example` with real owners.
- Restrict who can push to the protected branch.
