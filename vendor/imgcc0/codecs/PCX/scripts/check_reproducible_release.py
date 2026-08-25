#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import os
import shutil
import subprocess
import tempfile
from pathlib import Path


def digest_file(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run_build(repo_root: Path, make_cmd: str, version: str, out_dir: Path, epoch: int) -> None:
    env = os.environ.copy()
    env['SOURCE_DATE_EPOCH'] = str(epoch)
    subprocess.run([make_cmd, 'clean'], cwd=repo_root, check=True, env=env)
    subprocess.run(
        [
            'python3',
            'scripts/build_release_artifacts.py',
            '--project', args.project,
            '--version', version,
            '--repo-root', str(repo_root),
            '--dist-dir', str(out_dir),
            '--platform-tag', args.platform_tag,
            '--make', make_cmd,
        ],
        cwd=repo_root,
        check=True,
        env=env,
    )


parser = argparse.ArgumentParser(description='Build release artifacts twice and compare digests.')
parser.add_argument('--project', required=True)
parser.add_argument('--version', required=True)
parser.add_argument('--repo-root', required=True)
parser.add_argument('--platform-tag', required=True)
parser.add_argument('--make', default='make')
args = parser.parse_args()

repo_root = Path(args.repo_root).resolve()
epoch = int(os.environ.get('SOURCE_DATE_EPOCH', '1735689600'))

with tempfile.TemporaryDirectory(prefix=f'{args.project}-repro-a-') as tmp_a, tempfile.TemporaryDirectory(prefix=f'{args.project}-repro-b-') as tmp_b:
    dir_a = Path(tmp_a) / 'dist'
    dir_b = Path(tmp_b) / 'dist'
    run_build(repo_root, args.make, args.version, dir_a, epoch)
    run_build(repo_root, args.make, args.version, dir_b, epoch)

    files_a = sorted(p.name for p in dir_a.iterdir() if p.is_file())
    files_b = sorted(p.name for p in dir_b.iterdir() if p.is_file())
    if files_a != files_b:
        raise SystemExit('reproducibility check failed: artifact file set differs')

    for name in files_a:
        hash_a = digest_file(dir_a / name)
        hash_b = digest_file(dir_b / name)
        if hash_a != hash_b:
            raise SystemExit(f'reproducibility check failed: digest mismatch for {name}')

print('check_reproducible_release: OK')
