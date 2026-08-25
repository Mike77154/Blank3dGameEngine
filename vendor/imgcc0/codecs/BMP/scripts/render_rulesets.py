#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
from pathlib import Path


def load_required_checks(path: Path) -> list[str]:
    data = json.loads(path.read_text(encoding='utf-8'))
    checks = data.get('required_status_checks', [])
    if not isinstance(checks, list) or not checks or not all(isinstance(item, str) and item.strip() for item in checks):
        raise SystemExit('required_status_checks.json must define a non-empty list of strings under required_status_checks')
    return list(dict.fromkeys(checks))


def default_branch_ruleset(required_checks: list[str]) -> dict:
    return {
        'name': 'default-branch-protection',
        'target': 'branch',
        'enforcement': 'active',
        'bypass_actors': [],
        'conditions': {
            'ref_name': {
                'include': ['~DEFAULT_BRANCH'],
                'exclude': []
            }
        },
        'rules': [
            {'type': 'deletion'},
            {'type': 'non_fast_forward'},
            {'type': 'required_linear_history'},
            {'type': 'required_signatures'},
            {
                'type': 'pull_request',
                'parameters': {
                    'allowed_merge_methods': ['squash', 'rebase'],
                    'dismiss_stale_reviews_on_push': True,
                    'require_code_owner_review': True,
                    'require_last_push_approval': True,
                    'required_approving_review_count': 2,
                    'required_review_thread_resolution': True
                }
            },
            {
                'type': 'required_status_checks',
                'parameters': {
                    'do_not_enforce_on_create': False,
                    'required_status_checks': [{'context': item} for item in required_checks],
                    'strict_required_status_checks_policy': True
                }
            },
            {
                'type': 'merge_queue',
                'parameters': {
                    'check_response_timeout_minutes': 60,
                    'grouping_strategy': 'ALLGREEN',
                    'max_entries_to_build': 5,
                    'max_entries_to_merge': 5,
                    'merge_method': 'SQUASH',
                    'min_entries_to_merge': 1,
                    'min_entries_to_merge_wait_minutes': 5
                }
            }
        ]
    }


def release_tags_ruleset() -> dict:
    return {
        'name': 'release-tags-protection',
        'target': 'tag',
        'enforcement': 'active',
        'bypass_actors': [],
        'conditions': {
            'ref_name': {
                'include': ['v*'],
                'exclude': []
            }
        },
        'rules': [
            {
                'type': 'update',
                'parameters': {
                    'update_allows_fetch_and_merge': False
                }
            },
            {'type': 'deletion'}
        ]
    }


def main() -> int:
    parser = argparse.ArgumentParser(description='Render importable repository ruleset JSON files.')
    parser.add_argument('--required-checks', default='governance/required_status_checks.json')
    parser.add_argument('--output-dir', default='.github/rulesets')
    args = parser.parse_args()

    required_checks = load_required_checks(Path(args.required_checks))
    output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    (output_dir / 'default-branch-protection.json').write_text(
        json.dumps(default_branch_ruleset(required_checks), indent=2) + '\n',
        encoding='utf-8'
    )
    (output_dir / 'release-tags-protection.json').write_text(
        json.dumps(release_tags_ruleset(), indent=2) + '\n',
        encoding='utf-8'
    )
    print('render_rulesets: OK')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
