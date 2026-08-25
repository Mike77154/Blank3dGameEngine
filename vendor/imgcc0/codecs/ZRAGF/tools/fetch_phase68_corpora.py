#!/usr/bin/env python3
import argparse
import hashlib
import json
import os
import shutil
import sys
import tarfile
import urllib.request
import zipfile
from pathlib import Path


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open('rb') as f:
        while True:
            chunk = f.read(65536)
            if not chunk:
                break
            h.update(chunk)
    return h.hexdigest()


def download_first(urls, dst: Path, force: bool) -> str:
    dst.parent.mkdir(parents=True, exist_ok=True)
    if dst.exists() and not force:
        return 'cached'
    last = None
    tmp = dst.with_suffix(dst.suffix + '.tmp')
    if tmp.exists():
        tmp.unlink()
    for url in urls:
        try:
            with urllib.request.urlopen(url) as r, tmp.open('wb') as w:
                shutil.copyfileobj(r, w)
            tmp.replace(dst)
            return url
        except Exception as exc:  # pragma: no cover - network dependent
            last = exc
    raise RuntimeError(f'failed to download any URL for {dst.name}: {last}')


def extract_zip_member(archive: Path, member_suffix: str, out_path: Path) -> None:
    with zipfile.ZipFile(str(archive), 'r') as zf:
        for name in zf.namelist():
            if name.endswith(member_suffix):
                out_path.parent.mkdir(parents=True, exist_ok=True)
                with zf.open(name, 'r') as src, out_path.open('wb') as dst:
                    shutil.copyfileobj(src, dst)
                return
    raise RuntimeError(f'member not found in zip: {member_suffix}')


def extract_tar_member(archive: Path, member_suffix: str, out_path: Path) -> None:
    with tarfile.open(str(archive), 'r:*') as tf:
        for member in tf.getmembers():
            if member.name.endswith(member_suffix):
                extracted = tf.extractfile(member)
                if extracted is None:
                    raise RuntimeError(f'unreadable tar member: {member_suffix}')
                out_path.parent.mkdir(parents=True, exist_ok=True)
                with extracted, out_path.open('wb') as dst:
                    shutil.copyfileobj(extracted, dst)
                return
    raise RuntimeError(f'member not found in tar: {member_suffix}')


def collect_manifest_files(manifest: dict, root: Path):
    out = []
    for ds in manifest.get('datasets', []):
        kind = ds.get('kind')
        ds_root = root / ds['id']
        if kind == 'existing':
            base = root / ds.get('root', '')
            for item in ds.get('files', []):
                out.append(base / item['path'])
        elif kind == 'files':
            for item in ds.get('files', []):
                out.append(ds_root / item['output'])
        elif kind == 'archive':
            for item in ds.get('members', []):
                out.append(ds_root / item['output'])
    return out


def main() -> int:
    ap = argparse.ArgumentParser(description='Fetch phase 68 external corpora from a versioned manifest.')
    ap.add_argument('--manifest', required=True)
    ap.add_argument('--root', required=True)
    ap.add_argument('--force', action='store_true')
    ap.add_argument('--list', action='store_true', dest='list_only')
    args = ap.parse_args()

    manifest_path = Path(args.manifest)
    root = Path(args.root)
    manifest = json.loads(manifest_path.read_text(encoding='utf-8'))

    if args.list_only:
        for p in collect_manifest_files(manifest, root):
            print(p)
        return 0

    root.mkdir(parents=True, exist_ok=True)
    downloads = root / '_downloads'
    downloads.mkdir(parents=True, exist_ok=True)
    lock = {'manifest': str(manifest_path), 'root': str(root), 'files': []}

    for ds in manifest.get('datasets', []):
        ds_root = root / ds['id']
        kind = ds.get('kind')
        if kind == 'existing':
            base = root / ds.get('root', '')
            for item in ds.get('files', []):
                p = base / item['path']
                lock['files'].append({'dataset': ds['id'], 'path': str(p), 'exists': p.exists(), 'sha256': sha256_file(p) if p.exists() else None})
            continue
        if kind == 'files':
            for item in ds.get('files', []):
                out_path = ds_root / item['output']
                used = download_first(item['urls'], out_path, args.force)
                lock['files'].append({'dataset': ds['id'], 'path': str(out_path), 'source': used, 'size': out_path.stat().st_size, 'sha256': sha256_file(out_path)})
            continue
        if kind == 'archive':
            fmt = ds['archive']['format']
            url = ds['archive']['urls'][0]
            fname = url.rsplit('/', 1)[-1]
            archive_path = downloads / fname
            used = download_first(ds['archive']['urls'], archive_path, args.force)
            for item in ds.get('members', []):
                out_path = ds_root / item['output']
                if fmt == 'zip':
                    extract_zip_member(archive_path, item['member_suffix'], out_path)
                else:
                    extract_tar_member(archive_path, item['member_suffix'], out_path)
                lock['files'].append({'dataset': ds['id'], 'path': str(out_path), 'source': used, 'size': out_path.stat().st_size, 'sha256': sha256_file(out_path)})
            continue
        raise RuntimeError(f'unknown dataset kind: {kind}')

    lock_path = root / 'phase68_corpus_lock.json'
    lock_path.write_text(json.dumps(lock, indent=2) + '\n', encoding='utf-8')
    print(lock_path)
    return 0


if __name__ == '__main__':
    sys.exit(main())
