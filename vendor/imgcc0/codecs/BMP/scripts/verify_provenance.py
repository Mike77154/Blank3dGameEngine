#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
from pathlib import Path


def sha256_hex(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def verify_sig(openssl: str, public_key: Path, input_path: Path, sig_path: Path) -> None:
    subprocess.run([
        openssl, 'dgst', '-sha256', '-verify', str(public_key), '-signature', str(sig_path), str(input_path)
    ], check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)


def load_policy(path: Path | None) -> dict:
    if path is None:
        return {}
    return json.loads(path.read_text(encoding='utf-8'))


def locate_single(dist_dir: Path, suffix: str) -> Path:
    matches = sorted(dist_dir.glob(f'*{suffix}'))
    if len(matches) != 1:
        raise SystemExit(f'expected exactly one *{suffix} in {dist_dir}, found {len(matches)}')
    return matches[0]


def main() -> int:
    ap = argparse.ArgumentParser(description='Verify detached signatures and SLSA-like provenance for release artifacts.')
    ap.add_argument('--dist-dir', default='dist')
    ap.add_argument('--policy', default=None)
    ap.add_argument('--public-key', default=None)
    ap.add_argument('--expected-builder-id', default=None)
    ap.add_argument('--expected-build-type', default=None)
    ap.add_argument('--expected-project', default=None)
    ap.add_argument('--expected-version', default=None)
    ap.add_argument('--expected-platform', default=None)
    ap.add_argument('--openssl', default='openssl')
    args = ap.parse_args()

    dist_dir = Path(args.dist_dir).resolve()
    if not dist_dir.exists():
        raise SystemExit(f'missing dist dir: {dist_dir}')

    default_policy = dist_dir / 'verification-policy.json'
    policy = load_policy(Path(args.policy).resolve() if args.policy else (default_policy if default_policy.exists() else None))
    public_key = Path(args.public_key).resolve() if args.public_key else (dist_dir / policy.get('public_key') if policy.get('public_key') else locate_single(dist_dir, '-release-public.pem'))
    manifest_path = dist_dir / 'manifest.json'
    manifest_sig = dist_dir / policy.get('manifest_signature', 'manifest.json.sig')
    provenance_path = dist_dir / policy.get('provenance', locate_single(dist_dir, '-provenance.intoto.jsonl').name)
    provenance_sig = dist_dir / policy.get('provenance_signature', locate_single(dist_dir, '-provenance.intoto.jsonl.sig').name)

    if not public_key.exists():
        raise SystemExit(f'missing public key: {public_key}')
    for p in [manifest_path, manifest_sig, provenance_path, provenance_sig]:
        if not p.exists():
            raise SystemExit(f'missing verification input: {p}')

    verify_sig(args.openssl, public_key, manifest_path, manifest_sig)
    verify_sig(args.openssl, public_key, provenance_path, provenance_sig)

    manifest = json.loads(manifest_path.read_text(encoding='utf-8'))
    provenance = json.loads(provenance_path.read_text(encoding='utf-8'))

    if provenance.get('_type') != 'https://in-toto.io/Statement/v1':
        raise SystemExit('provenance: unexpected statement type')
    if provenance.get('predicateType') != 'https://slsa.dev/provenance/v1':
        raise SystemExit('provenance: unexpected predicateType')

    predicate = provenance.get('predicate', {})
    build_def = predicate.get('buildDefinition', {})
    run_details = predicate.get('runDetails', {})
    external = build_def.get('externalParameters', {})
    builder_id = run_details.get('builder', {}).get('id')
    build_type = build_def.get('buildType')

    expected_builder = args.expected_builder_id or (policy.get('trusted_builder_ids') or [None])[0]
    if expected_builder and builder_id != expected_builder:
        raise SystemExit(f'provenance: builder id mismatch: {builder_id!r} != {expected_builder!r}')
    expected_build_type = args.expected_build_type or policy.get('expected_build_type')
    if expected_build_type and build_type != expected_build_type:
        raise SystemExit(f'provenance: build type mismatch: {build_type!r} != {expected_build_type!r}')

    for key, expected in {
        'project': args.expected_project or policy.get('expected_external_parameters', {}).get('project'),
        'version': args.expected_version or policy.get('expected_external_parameters', {}).get('version'),
        'platform': args.expected_platform or policy.get('expected_external_parameters', {}).get('platform'),
    }.items():
        if expected and external.get(key) != expected:
            raise SystemExit(f'provenance: external parameter {key} mismatch: {external.get(key)!r} != {expected!r}')

    subjects = {item['name']: item['digest']['sha256'] for item in provenance.get('subject', [])}
    required_subjects = policy.get('required_subjects', sorted(subjects.keys()))
    missing = [name for name in required_subjects if name not in subjects]
    if missing:
        raise SystemExit(f'provenance: missing required subjects: {missing}')

    for name, expected_sha in subjects.items():
        path = dist_dir / name
        if not path.exists():
            raise SystemExit(f'provenance: missing subject file {name}')
        actual_sha = sha256_hex(path)
        if actual_sha != expected_sha:
            raise SystemExit(f'provenance: digest mismatch for {name}')

    for item in manifest.get('artifacts', []):
        name = item['name']
        path = dist_dir / name
        if not path.exists():
            raise SystemExit(f'manifest: missing artifact {name}')
        if sha256_hex(path) != item['sha256']:
            raise SystemExit(f'manifest: sha256 mismatch for {name}')

    print(f'verify_provenance: OK ({dist_dir})')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
