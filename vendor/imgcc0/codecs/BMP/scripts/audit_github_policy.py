#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import re
from pathlib import Path

import yaml

HEX40 = re.compile(r'^[0-9a-f]{40}$')


def load_yaml(path: Path) -> dict:
    try:
        return yaml.safe_load(path.read_text(encoding='utf-8')) or {}
    except Exception as exc:
        raise SystemExit(f'failed to parse {path}: {exc}')


def iter_uses(doc: dict):
    jobs = doc.get('jobs', {}) or {}
    for job_name, job in jobs.items():
        if not isinstance(job, dict):
            continue
        if 'uses' in job:
            yield path(f'jobs.{job_name}.uses'), job['uses'], None
        for idx, step in enumerate(job.get('steps', []) or []):
            if isinstance(step, dict) and 'uses' in step:
                yield path(f'jobs.{job_name}.steps[{idx}].uses'), step['uses'], step


def path(value: str) -> str:
    return value


def split_action_ref(value: str):
    if '@' not in value:
        return value, ''
    name, ref = value.rsplit('@', 1)
    return name, ref


def render_markdown(result: dict) -> str:
    lines = ['# GitHub policy audit', '']
    lines.append(f"Errors: {len(result['errors'])}")
    lines.append(f"Warnings: {len(result['warnings'])}")
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
    lines.append('## Workflow action inventory')
    for wf in result['workflows']:
        lines.append(f"- `{wf['file']}`: {', '.join(wf['actions']) if wf['actions'] else 'no external actions'}")
    lines.append('')
    return '\n'.join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(description='Audit workflow immutability and governance policy.')
    parser.add_argument('--repo-root', default='.')
    parser.add_argument('--approved-actions', required=True)
    parser.add_argument('--json-out')
    parser.add_argument('--md-out')
    args = parser.parse_args()

    repo_root = Path(args.repo_root).resolve()
    approved = json.loads(Path(args.approved_actions).read_text(encoding='utf-8'))

    workflows_dir = repo_root / '.github' / 'workflows'
    workflow_files = sorted(list(workflows_dir.glob('*.yml')) + list(workflows_dir.glob('*.yaml')))
    errors = []
    warnings = []
    workflows = []

    if not workflow_files:
        errors.append({'where': '.github/workflows', 'message': 'no workflow files found'})

    codeowners_path = repo_root / '.github' / 'CODEOWNERS'
    codeowners_example = repo_root / '.github' / 'CODEOWNERS.example'
    if not codeowners_path.exists():
        if codeowners_example.exists():
            warnings.append({'where': '.github/CODEOWNERS.example', 'message': 'template exists, but a real CODEOWNERS file is not configured yet'})
        else:
            warnings.append({'where': '.github', 'message': 'missing CODEOWNERS or CODEOWNERS.example'})

    pr_template = repo_root / '.github' / 'pull_request_template.md'
    if not pr_template.exists():
        warnings.append({'where': '.github/pull_request_template.md', 'message': 'missing PR template'})

    for wf_path in workflow_files:
        doc = load_yaml(wf_path)
        used_actions = []
        if 'permissions' not in doc:
            errors.append({'where': str(wf_path.relative_to(repo_root)), 'message': 'missing top-level permissions block'})
        if 'concurrency' not in doc:
            errors.append({'where': str(wf_path.relative_to(repo_root)), 'message': 'missing top-level concurrency block'})
        jobs = doc.get('jobs', {}) or {}
        if not jobs:
            errors.append({'where': str(wf_path.relative_to(repo_root)), 'message': 'workflow has no jobs'})
        for job_name, job in jobs.items():
            if not isinstance(job, dict):
                continue
            if 'permissions' not in job:
                errors.append({'where': f'{wf_path.relative_to(repo_root)}::{job_name}', 'message': 'missing explicit job permissions'})
            if 'timeout-minutes' not in job:
                errors.append({'where': f'{wf_path.relative_to(repo_root)}::{job_name}', 'message': 'missing job timeout-minutes'})
        for where, value, step in iter_uses(doc):
            if not isinstance(value, str):
                continue
            name, ref = split_action_ref(value)
            if name.startswith('./') or value.startswith('docker://'):
                continue
            used_actions.append(name)
            approved_item = approved.get(name)
            if approved_item is None:
                errors.append({'where': f'{wf_path.relative_to(repo_root)}::{where}', 'message': f'action {name!r} is not in policies/approved_actions.json'})
                continue
            if not HEX40.match(ref):
                errors.append({'where': f'{wf_path.relative_to(repo_root)}::{where}', 'message': 'action is not pinned to a full 40-character commit SHA'})
            if ref != approved_item['sha']:
                errors.append({'where': f'{wf_path.relative_to(repo_root)}::{where}', 'message': f'action ref {ref} does not match approved sha {approved_item["sha"]}'})
            if name == 'actions/checkout' and isinstance(step, dict):
                with_block = step.get('with', {}) or {}
                if with_block.get('persist-credentials') is not False:
                    errors.append({'where': f'{wf_path.relative_to(repo_root)}::{where}', 'message': 'actions/checkout must set persist-credentials: false'})
        workflows.append({'file': str(wf_path.relative_to(repo_root)), 'actions': used_actions})

    result = {'errors': errors, 'warnings': warnings, 'workflows': workflows}

    if args.json_out:
        out = Path(args.json_out)
        out.parent.mkdir(parents=True, exist_ok=True)
        out.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    if args.md_out:
        out = Path(args.md_out)
        out.parent.mkdir(parents=True, exist_ok=True)
        out.write_text(render_markdown(result), encoding='utf-8')

    if errors:
        raise SystemExit('github policy audit failed')

    print('audit_github_policy: OK')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
