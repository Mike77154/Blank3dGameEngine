#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any

import yaml


def load_json(path: Path) -> Any:
    return json.loads(path.read_text(encoding='utf-8'))


def load_yaml(path: Path) -> Any:
    return yaml.safe_load(path.read_text(encoding='utf-8')) or {}


def find_rule(rules: list[dict[str, Any]], rule_type: str) -> dict[str, Any] | None:
    for rule in rules:
        if isinstance(rule, dict) and rule.get('type') == rule_type:
            return rule
    return None


def render_markdown(result: dict[str, Any]) -> str:
    lines = ['# Repository governance audit', '']
    lines.append(f"Errors: {len(result['errors'])}")
    lines.append(f"Warnings: {len(result['warnings'])}")
    lines.append('')
    lines.append('## Required status checks')
    for item in result['required_status_checks']:
        lines.append(f'- `{item}`')
    lines.append('')
    if result['errors']:
        lines.append('## Errors')
        for item in result['errors']:
            lines.append(f"- `{item['where']}`: {item['message']}")
        lines.append('')
    if result['warnings']:
        lines.append('## Warnings')
        for item in result['warnings']:
            lines.append(f"- `{item['where']}`: {item['message']}")
        lines.append('')
    return '\n'.join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(description='Audit rulesets-as-code and release governance wiring.')
    parser.add_argument('--repo-root', default='.')
    parser.add_argument('--required-checks', default='governance/required_status_checks.json')
    parser.add_argument('--json-out')
    parser.add_argument('--md-out')
    args = parser.parse_args()

    repo_root = Path(args.repo_root).resolve()
    errors: list[dict[str, str]] = []
    warnings: list[dict[str, str]] = []

    required_checks_data = load_json(repo_root / args.required_checks)
    required_checks = required_checks_data.get('required_status_checks', [])
    if not isinstance(required_checks, list) or not required_checks:
        errors.append({'where': args.required_checks, 'message': 'missing required_status_checks list'})
        required_checks = []

    branch_ruleset_path = repo_root / '.github' / 'rulesets' / 'default-branch-protection.json'
    tag_ruleset_path = repo_root / '.github' / 'rulesets' / 'release-tags-protection.json'
    desired_state_path = repo_root / 'governance' / 'repository_desired_state.json'
    release_env_path = repo_root / 'governance' / 'environments' / 'release.json'

    for path in [branch_ruleset_path, tag_ruleset_path, desired_state_path, release_env_path, repo_root / 'MAINTAINERS.md', repo_root / 'SUPPORT.md', repo_root / '.github' / 'ISSUE_TEMPLATE' / 'bug_report.yml', repo_root / '.github' / 'ISSUE_TEMPLATE' / 'config.yml']:
        if not path.exists():
            errors.append({'where': str(path.relative_to(repo_root)), 'message': 'missing governance artifact'})

    if branch_ruleset_path.exists():
        branch_ruleset = load_json(branch_ruleset_path)
        if branch_ruleset.get('target') != 'branch':
            errors.append({'where': str(branch_ruleset_path.relative_to(repo_root)), 'message': 'target must be branch'})
        if branch_ruleset.get('enforcement') != 'active':
            errors.append({'where': str(branch_ruleset_path.relative_to(repo_root)), 'message': 'ruleset must be active'})
        include = (((branch_ruleset.get('conditions') or {}).get('ref_name') or {}).get('include') or [])
        if '~DEFAULT_BRANCH' not in include:
            errors.append({'where': str(branch_ruleset_path.relative_to(repo_root)), 'message': 'ruleset must target ~DEFAULT_BRANCH'})
        rules = branch_ruleset.get('rules', []) or []
        for rule_type in ['deletion', 'non_fast_forward', 'required_linear_history', 'required_signatures', 'pull_request', 'required_status_checks', 'merge_queue']:
            if find_rule(rules, rule_type) is None:
                errors.append({'where': str(branch_ruleset_path.relative_to(repo_root)), 'message': f'missing rule {rule_type}'})
        pr_rule = find_rule(rules, 'pull_request') or {}
        pr_params = pr_rule.get('parameters', {}) if isinstance(pr_rule, dict) else {}
        if pr_params.get('required_approving_review_count', 0) < 2:
            errors.append({'where': str(branch_ruleset_path.relative_to(repo_root)), 'message': 'pull_request rule must require at least 2 approvals'})
        for key in ['dismiss_stale_reviews_on_push', 'require_code_owner_review', 'require_last_push_approval', 'required_review_thread_resolution']:
            if pr_params.get(key) is not True:
                errors.append({'where': str(branch_ruleset_path.relative_to(repo_root)), 'message': f'pull_request.{key} must be true'})
        status_rule = find_rule(rules, 'required_status_checks') or {}
        status_params = status_rule.get('parameters', {}) if isinstance(status_rule, dict) else {}
        contexts = [item.get('context') for item in (status_params.get('required_status_checks') or []) if isinstance(item, dict)]
        if contexts != required_checks:
            errors.append({'where': str(branch_ruleset_path.relative_to(repo_root)), 'message': f'required status checks mismatch: expected {required_checks!r}, found {contexts!r}'})
        if status_params.get('strict_required_status_checks_policy') is not True:
            errors.append({'where': str(branch_ruleset_path.relative_to(repo_root)), 'message': 'strict required status checks policy must be true'})
        merge_rule = find_rule(rules, 'merge_queue') or {}
        merge_params = merge_rule.get('parameters', {}) if isinstance(merge_rule, dict) else {}
        if merge_params.get('merge_method') != 'SQUASH':
            errors.append({'where': str(branch_ruleset_path.relative_to(repo_root)), 'message': 'merge queue must use SQUASH'})
        if int(merge_params.get('min_entries_to_merge', 0) or 0) < 1:
            errors.append({'where': str(branch_ruleset_path.relative_to(repo_root)), 'message': 'merge queue min_entries_to_merge must be >= 1'})

    if tag_ruleset_path.exists():
        tag_ruleset = load_json(tag_ruleset_path)
        if tag_ruleset.get('target') != 'tag':
            errors.append({'where': str(tag_ruleset_path.relative_to(repo_root)), 'message': 'target must be tag'})
        include = (((tag_ruleset.get('conditions') or {}).get('ref_name') or {}).get('include') or [])
        if 'v*' not in include:
            errors.append({'where': str(tag_ruleset_path.relative_to(repo_root)), 'message': 'tag ruleset must protect v*'})
        rules = tag_ruleset.get('rules', []) or []
        for rule_type in ['update', 'deletion']:
            if find_rule(rules, rule_type) is None:
                errors.append({'where': str(tag_ruleset_path.relative_to(repo_root)), 'message': f'missing rule {rule_type}'})

    ci_workflow = load_yaml(repo_root / '.github' / 'workflows' / 'ci.yml')
    codeql_workflow = load_yaml(repo_root / '.github' / 'workflows' / 'codeql.yml')
    release_workflow = load_yaml(repo_root / '.github' / 'workflows' / 'release.yml')

    for workflow_name, workflow in [('ci.yml', ci_workflow), ('codeql.yml', codeql_workflow)]:
        on_block = workflow.get('on')
        if on_block is None:
            on_block = workflow.get(True)
        normalized = on_block if isinstance(on_block, dict) else {}
        if 'merge_group' not in normalized:
            errors.append({'where': workflow_name, 'message': 'workflow must listen to merge_group for merge queue compatibility'})

    merge_gate_job = (ci_workflow.get('jobs') or {}).get('merge-gate')
    if not isinstance(merge_gate_job, dict):
        errors.append({'where': '.github/workflows/ci.yml', 'message': 'missing merge-gate job'})
    else:
        expected_needs = ['build-and-test', 'sanitizers', 'observability', 'supply-chain', 'package-ecosystem', 'portability-matrix', 'analysis-contracts']
        if merge_gate_job.get('needs') != expected_needs:
            errors.append({'where': '.github/workflows/ci.yml', 'message': f'merge-gate needs must be {expected_needs!r}'})

    release_job = (release_workflow.get('jobs') or {}).get('release')
    if not isinstance(release_job, dict) or release_job.get('environment') != 'release':
        errors.append({'where': '.github/workflows/release.yml', 'message': 'release job must reference the release environment'})

    if release_env_path.exists():
        env_data = load_json(release_env_path)
        if env_data.get('name') != 'release':
            errors.append({'where': str(release_env_path.relative_to(repo_root)), 'message': 'release environment file must be named release'})

    result = {
        'required_status_checks': required_checks,
        'errors': errors,
        'warnings': warnings,
    }

    if args.json_out:
        out = Path(args.json_out)
        out.parent.mkdir(parents=True, exist_ok=True)
        out.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    if args.md_out:
        out = Path(args.md_out)
        out.parent.mkdir(parents=True, exist_ok=True)
        out.write_text(render_markdown(result), encoding='utf-8')

    if errors:
        raise SystemExit('repository governance audit failed')

    print('audit_repo_governance: OK')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
