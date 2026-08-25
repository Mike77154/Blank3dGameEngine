#!/usr/bin/env python3
import json
import shutil
import subprocess
import sys
import tarfile
import tempfile
import zipfile
from pathlib import Path


def write_manifest(path: Path) -> None:
    payload = {
        'manifest_version': 2,
        'name': 'phase70-materialize-test',
        'datasets': [
            {
                'id': 'zip-ds',
                'kind': 'archive',
                'family': 'mixed',
                'version': 'test-zip',
                'archive': {
                    'format': 'zip',
                    'urls': ['https://example.invalid/cantrbry.zip'],
                    'cache_names': ['cantrbry.zip'],
                    'expected_sha256': None,
                    'expected_size': None,
                },
                'members': [
                    {'member_suffix': 'alpha.txt', 'output': 'alpha.txt', 'expected_sha256': None, 'expected_size': None},
                    {'member_suffix': 'binary.bin', 'output': 'binary.bin', 'expected_sha256': None, 'expected_size': None},
                ],
            },
            {
                'id': 'tar-ds',
                'kind': 'archive',
                'family': 'tiff',
                'version': 'test-tar',
                'archive': {
                    'format': 'tar.gz',
                    'urls': ['https://example.invalid/pics-3.8.0.tar.gz'],
                    'cache_names': ['pics-3.8.0.tar.gz'],
                    'expected_sha256': None,
                    'expected_size': None,
                },
                'members': [
                    {'member_suffix': 'strip_gray.tiff', 'output': 'strip_gray.tiff', 'expected_sha256': None, 'expected_size': None},
                ],
            },
            {
                'id': 'file-ds',
                'kind': 'files',
                'family': 'png',
                'version': 'test-files',
                'files': [
                    {'output': 'sprite_rgba.png', 'urls': ['https://example.invalid/sprite_rgba.png'], 'cache_names': ['sprite_rgba.png'], 'expected_sha256': None, 'expected_size': None},
                ],
            },
        ],
    }
    path.write_text(json.dumps(payload, indent=2) + '\n', encoding='utf-8')


def main() -> int:
    root = Path(__file__).resolve().parents[1]
    with tempfile.TemporaryDirectory(prefix='zragf70-cache-') as tmpdir:
        tmp = Path(tmpdir)
        cache = tmp / 'cache'
        root_out = tmp / 'materialized'
        cache.mkdir(parents=True, exist_ok=True)

        src_root = root / 'bench' / 'corpora' / 'local_phase70'
        with zipfile.ZipFile(str(cache / 'cantrbry.zip'), 'w', compression=zipfile.ZIP_DEFLATED) as zf:
            zf.write(str(src_root / 'alpha.txt'), arcname='cantrbry/alpha.txt')
            zf.write(str(src_root / 'binary.bin'), arcname='cantrbry/binary.bin')
        with tarfile.open(str(cache / 'pics-3.8.0.tar.gz'), 'w:gz') as tf:
            tf.add(str(src_root / 'strip_gray.tiff'), arcname='pics-3.8.0/images/strip_gray.tiff')
        shutil.copy2(src_root / 'sprite_rgba.png', cache / 'sprite_rgba.png')

        manifest = tmp / 'manifest.json'
        lock = tmp / 'phase70_corpus_lock.json'
        frozen = tmp / 'phase70_frozen_manifest.json'
        write_manifest(manifest)

        cmd = [
            sys.executable,
            str(root / 'tools' / 'fetch_phase70_corpora.py'),
            '--manifest', str(manifest),
            '--root', str(root_out),
            '--archive-cache', str(cache),
            '--cache-only',
            '--lock-out', str(lock),
            '--freeze-manifest-out', str(frozen),
        ]
        proc = subprocess.run(cmd, cwd=str(root), check=False)
        if proc.returncode != 0:
            return proc.returncode
        if not lock.exists() or not frozen.exists():
            print('missing outputs', file=sys.stderr)
            return 1
        payload = json.loads(lock.read_text(encoding='utf-8'))
        if len(payload.get('datasets', [])) != 3:
            print('unexpected dataset count', file=sys.stderr)
            return 1
        if not (root_out / 'zip-ds' / 'alpha.txt').exists():
            print('zip member missing', file=sys.stderr)
            return 1
        if not (root_out / 'tar-ds' / 'strip_gray.tiff').exists():
            print('tar member missing', file=sys.stderr)
            return 1
        if not (root_out / 'file-ds' / 'sprite_rgba.png').exists():
            print('file member missing', file=sys.stderr)
            return 1
        frozen_payload = json.loads(frozen.read_text(encoding='utf-8'))
        if not frozen_payload['datasets'][0]['members'][0].get('expected_sha256'):
            print('frozen manifest missing hashes', file=sys.stderr)
            return 1
    print('phase70 materialize cached ok')
    return 0


if __name__ == '__main__':
    sys.exit(main())
