#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import os
import sys
import urllib.error
import urllib.parse
import urllib.request
from pathlib import Path


API_ROOT = 'https://api.github.com'


def load_json(path: Path) -> dict:
    return json.loads(path.read_text(encoding='utf-8'))


def request_json(method: str, url: str, token: str, body: dict | None = None):
    data = None
    if body is not None:
        data = json.dumps(body).encode('utf-8')
    request = urllib.request.Request(url, data=data, method=method)
    request.add_header('Accept', 'application/vnd.github+json')
    request.add_header('Authorization', f'Bearer {token}')
    request.add_header('X-GitHub-Api-Version', '2022-11-28')
    request.add_header('User-Agent', 'codec-governance-tooling/1')
    if data is not None:
        request.add_header('Content-Type', 'application/json')
    with urllib.request.urlopen(request) as response:
        payload = response.read()
        if not payload:
            return None
        return json.loads(payload.decode('utf-8'))


def build_plan(owner: str, repo: str, repo_root: Path) -> list[tuple[str, str, str, dict | None]]:
    rulesets = [
        repo_root / '.github' / 'rulesets' / 'default-branch-protection.json',
        repo_root / '.github' / 'rulesets' / 'release-tags-protection.json',
    ]
    plan = []
    for ruleset_path in rulesets:
        payload = load_json(ruleset_path)
        plan.append(('UPSERT_RULESET', ruleset_path.name, f'/repos/{owner}/{repo}/rulesets', payload))
    plan.append(('ENABLE_IMMUTABLE_RELEASES', 'immutable-releases', f'/repos/{owner}/{repo}/immutable-releases', None))
    return plan


def write_dry_run(plan: list[tuple[str, str, str, dict | None]], repo_root: Path, out_path: Path | None):
    lines = ['#!/usr/bin/env bash', 'set -euo pipefail', '']
    lines.append('OWNER=${OWNER:-REPLACE_ME_OWNER}')
    lines.append('REPO=${REPO:-REPLACE_ME_REPO}')
    lines.append('')
    for action, name, endpoint, payload in plan:
        endpoint = endpoint.replace(repo_root.name, repo_root.name)
        if action == 'UPSERT_RULESET':
            relative = '.github/rulesets/' + name
            lines.append(f"echo 'Create or update ruleset from {relative}'")
            lines.append(f"gh api repos/$OWNER/$REPO/rulesets --method POST --input {relative}")
        elif action == 'ENABLE_IMMUTABLE_RELEASES':
            lines.append("echo 'Enable immutable releases'")
            lines.append('gh api repos/$OWNER/$REPO/immutable-releases --method PUT')
        lines.append('')
    content = '\n'.join(lines) + '\n'
    if out_path is not None:
        out_path.parent.mkdir(parents=True, exist_ok=True)
        out_path.write_text(content, encoding='utf-8')
    else:
        sys.stdout.write(content)


def apply_plan(owner: str, repo: str, repo_root: Path, token: str):
    ruleset_paths = [
        repo_root / '.github' / 'rulesets' / 'default-branch-protection.json',
        repo_root / '.github' / 'rulesets' / 'release-tags-protection.json',
    ]
    existing = request_json('GET', f'{API_ROOT}/repos/{owner}/{repo}/rulesets?includes_parents=false&targets=branch,tag', token) or []
    existing_by_key = {(item.get('name'), item.get('target')): item for item in existing if isinstance(item, dict)}
    for ruleset_path in ruleset_paths:
        payload = load_json(ruleset_path)
        key = (payload.get('name'), payload.get('target'))
        existing_item = existing_by_key.get(key)
        if existing_item is None:
            request_json('POST', f'{API_ROOT}/repos/{owner}/{repo}/rulesets', token, payload)
            print(f'created ruleset {payload.get("name")}')
        else:
            request_json('PUT', f'{API_ROOT}/repos/{owner}/{repo}/rulesets/{existing_item["id"]}', token, payload)
            print(f'updated ruleset {payload.get("name")}')
    request_json('PUT', f'{API_ROOT}/repos/{owner}/{repo}/immutable-releases', token)
    print('enabled immutable releases')


def main() -> int:
    parser = argparse.ArgumentParser(description='Create/update repository rulesets and immutable releases policy.')
    parser.add_argument('--repo-root', default='.')
    parser.add_argument('--owner', required=True)
    parser.add_argument('--repo', required=True)
    parser.add_argument('--token-env', default='GITHUB_TOKEN')
    parser.add_argument('--dry-run', action='store_true')
    parser.add_argument('--out')
    args = parser.parse_args()

    repo_root = Path(args.repo_root).resolve()
    plan = build_plan(args.owner, args.repo, repo_root)
    if args.dry_run:
        write_dry_run(plan, repo_root, Path(args.out).resolve() if args.out else None)
        return 0

    token = os.environ.get(args.token_env)
    if not token:
        raise SystemExit(f'expected token in environment variable {args.token_env}')
    try:
        apply_plan(args.owner, args.repo, repo_root, token)
    except urllib.error.HTTPError as exc:
        body = exc.read().decode('utf-8', errors='replace')
        raise SystemExit(f'GitHub API request failed: {exc.code} {exc.reason}: {body}')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
