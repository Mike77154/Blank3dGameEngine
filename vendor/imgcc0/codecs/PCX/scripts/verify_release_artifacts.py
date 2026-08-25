#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import tarfile
import zipfile
from pathlib import Path


def parse_checksums(path: Path) -> dict[str, str]:
    values: dict[str, str] = {}
    for line in path.read_text(encoding='utf-8').splitlines():
        line = line.strip()
        if not line:
            continue
        digest, name = line.split('  ', 1)
        values[name] = digest
    return values


def verify_checksum_file(dist_dir: Path, filename: str, algorithm: str) -> None:
    mapping = parse_checksums(dist_dir / filename)
    for name, expected in mapping.items():
        target = dist_dir / name
        actual = getattr(hashlib, algorithm)(target.read_bytes()).hexdigest()
        if actual != expected:
            raise SystemExit(f'{filename}: checksum mismatch for {name}')


def verify_manifest(dist_dir: Path) -> dict:
    manifest = json.loads((dist_dir / 'manifest.json').read_text(encoding='utf-8'))
    listed = {item['name']: item for item in manifest['artifacts']}
    for name, item in listed.items():
        path = dist_dir / name
        if not path.exists():
            raise SystemExit(f'manifest.json: missing artifact {name}')
        actual_sha256 = hashlib.sha256(path.read_bytes()).hexdigest()
        actual_sha512 = hashlib.sha512(path.read_bytes()).hexdigest()
        if actual_sha256 != item['sha256'] or actual_sha512 != item['sha512']:
            raise SystemExit(f'manifest.json: digest mismatch for {name}')
        if path.stat().st_size != item['size']:
            raise SystemExit(f'manifest.json: size mismatch for {name}')
    return manifest


def verify_archives(dist_dir: Path, manifest: dict) -> None:
    for item in manifest['artifacts']:
        name = item['name']
        path = dist_dir / name
        if name.endswith('.tar.gz'):
            with tarfile.open(path, 'r:gz') as tf:
                members = tf.getmembers()
                if not members:
                    raise SystemExit(f'{name}: archive is empty')
                top_dirs = {member.name.split('/', 1)[0] for member in members}
                if len(top_dirs) != 1:
                    raise SystemExit(f'{name}: expected a single top-level directory')
        elif name.endswith('.zip'):
            with zipfile.ZipFile(path, 'r') as zf:
                names = zf.namelist()
                if not names:
                    raise SystemExit(f'{name}: archive is empty')
                top_dirs = {member.split('/', 1)[0] for member in names}
                if len(top_dirs) != 1:
                    raise SystemExit(f'{name}: expected a single top-level directory')
        elif name.endswith('.cdx.json'):
            doc = json.loads(path.read_text(encoding='utf-8'))
            if doc.get('bomFormat') != 'CycloneDX':
                raise SystemExit(f'{name}: expected CycloneDX document')
            if 'components' not in doc:
                raise SystemExit(f'{name}: missing components array')


def main() -> int:
    parser = argparse.ArgumentParser(description='Verify release artifacts, checksums, and SBOMs.')
    parser.add_argument('--dist-dir', default='dist')
    args = parser.parse_args()

    dist_dir = Path(args.dist_dir).resolve()
    if not dist_dir.exists():
        raise SystemExit(f'missing dist dir: {dist_dir}')

    manifest = verify_manifest(dist_dir)
    verify_checksum_file(dist_dir, 'SHA256SUMS', 'sha256')
    verify_checksum_file(dist_dir, 'SHA512SUMS', 'sha512')
    verify_archives(dist_dir, manifest)
    print(f'verify_release_artifacts: OK ({dist_dir})')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
