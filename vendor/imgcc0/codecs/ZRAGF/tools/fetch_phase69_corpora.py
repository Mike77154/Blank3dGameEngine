#!/usr/bin/env python3
import argparse
import datetime as _dt
import hashlib
import json
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


def verify_hash(path: Path, expected: str, label: str) -> None:
    if not expected:
        return
    got = sha256_file(path)
    if got.lower() != expected.lower():
        raise RuntimeError('%s sha256 mismatch for %s: expected %s got %s' % (label, path, expected, got))


def verify_size(path: Path, expected_size) -> None:
    if expected_size is None:
        return
    got = path.stat().st_size
    if int(expected_size) != int(got):
        raise RuntimeError('size mismatch for %s: expected %s got %s' % (path, expected_size, got))


def download_first(urls, dst: Path, force: bool) -> str:
    dst.parent.mkdir(parents=True, exist_ok=True)
    if dst.exists() and not force:
        return 'cached'
    tmp = dst.with_suffix(dst.suffix + '.tmp')
    if tmp.exists():
        tmp.unlink()
    last = None
    for url in urls:
        try:
            with urllib.request.urlopen(url) as src, tmp.open('wb') as out:
                shutil.copyfileobj(src, out)
            tmp.replace(dst)
            return url
        except Exception as exc:  # pragma: no cover - network dependent
            last = exc
    raise RuntimeError('failed to download %s: %r' % (dst.name, last))


def extract_zip_member(archive: Path, member_suffix: str, out_path: Path) -> None:
    with zipfile.ZipFile(str(archive), 'r') as zf:
        for name in zf.namelist():
            if name.endswith(member_suffix):
                out_path.parent.mkdir(parents=True, exist_ok=True)
                with zf.open(name, 'r') as src, out_path.open('wb') as dst:
                    shutil.copyfileobj(src, dst)
                return
    raise RuntimeError('zip member not found: %s' % member_suffix)


def extract_tar_member(archive: Path, member_suffix: str, out_path: Path) -> None:
    with tarfile.open(str(archive), 'r:*') as tf:
        for member in tf.getmembers():
            if member.name.endswith(member_suffix):
                src = tf.extractfile(member)
                if src is None:
                    raise RuntimeError('tar member unreadable: %s' % member_suffix)
                out_path.parent.mkdir(parents=True, exist_ok=True)
                with src, out_path.open('wb') as dst:
                    shutil.copyfileobj(src, dst)
                return
    raise RuntimeError('tar member not found: %s' % member_suffix)


def collect_files_from_manifest(manifest: dict, root: Path):
    out = []
    for ds in manifest.get('datasets', []):
        kind = ds.get('kind')
        if kind == 'existing':
            base = root / ds.get('root', '')
            for item in ds.get('files', []):
                out.append(base / item['path'])
        elif kind == 'files':
            ds_root = root / ds['id']
            for item in ds.get('files', []):
                out.append(ds_root / item['output'])
        elif kind == 'archive':
            ds_root = root / ds['id']
            for item in ds.get('members', []):
                out.append(ds_root / item['output'])
    return out


def build_frozen_manifest(source_manifest: dict, lock: dict) -> dict:
    frozen = {
        'manifest_version': source_manifest.get('manifest_version', 1),
        'name': source_manifest.get('name', '') + '-frozen',
        'description': 'Frozen content-hash manifest derived from lockfile generation.',
        'datasets': []
    }
    lock_by_dataset = {}
    for ds in lock.get('datasets', []):
        lock_by_dataset[ds['id']] = ds
    for ds in source_manifest.get('datasets', []):
        frozen_ds = dict(ds)
        frozen_lock = lock_by_dataset.get(ds['id'], {'files': []})
        files_by_output = {}
        for f in frozen_lock.get('files', []):
            rel = f.get('relative_path') or f.get('output') or Path(f['path']).name
            files_by_output[rel] = f
        if ds.get('kind') == 'files':
            new_files = []
            for item in ds.get('files', []):
                entry = dict(item)
                locked = files_by_output.get(item['output'])
                if locked:
                    entry['expected_sha256'] = locked.get('sha256')
                    entry['expected_size'] = locked.get('size')
                new_files.append(entry)
            frozen_ds['files'] = new_files
        elif ds.get('kind') == 'archive':
            frozen_archive = dict(ds['archive'])
            archive_lock = frozen_lock.get('archive')
            if archive_lock:
                frozen_archive['expected_sha256'] = archive_lock.get('sha256')
                frozen_archive['expected_size'] = archive_lock.get('size')
            frozen_ds['archive'] = frozen_archive
            new_members = []
            for item in ds.get('members', []):
                entry = dict(item)
                locked = files_by_output.get(item['output'])
                if locked:
                    entry['expected_sha256'] = locked.get('sha256')
                    entry['expected_size'] = locked.get('size')
                new_members.append(entry)
            frozen_ds['members'] = new_members
        elif ds.get('kind') == 'existing':
            new_files = []
            for item in ds.get('files', []):
                entry = dict(item)
                locked = files_by_output.get(item['path']) or files_by_output.get(Path(item['path']).name)
                if locked:
                    entry['expected_sha256'] = locked.get('sha256')
                    entry['expected_size'] = locked.get('size')
                new_files.append(entry)
            frozen_ds['files'] = new_files
        frozen['datasets'].append(frozen_ds)
    return frozen


def main() -> int:
    ap = argparse.ArgumentParser(description='Fetch or verify phase 69 corpora, emit a content-hash lockfile, and optionally freeze a manifest.')
    ap.add_argument('--manifest', required=True)
    ap.add_argument('--root', required=True)
    ap.add_argument('--force', action='store_true')
    ap.add_argument('--lock-out')
    ap.add_argument('--freeze-manifest-out')
    ap.add_argument('--list', action='store_true', dest='list_only')
    args = ap.parse_args()

    manifest_path = Path(args.manifest)
    root = Path(args.root)
    manifest = json.loads(manifest_path.read_text(encoding='utf-8'))
    if args.list_only:
        for path in collect_files_from_manifest(manifest, root):
            print(path)
        return 0

    root.mkdir(parents=True, exist_ok=True)
    downloads = root / '_downloads'
    downloads.mkdir(parents=True, exist_ok=True)
    lock = {
        'phase': 69,
        'created_utc': _dt.datetime.now(_dt.timezone.utc).replace(microsecond=0).isoformat().replace('+00:00', 'Z'),
        'manifest': str(manifest_path),
        'root': str(root),
        'datasets': []
    }

    for ds in manifest.get('datasets', []):
        ds_root = root / ds['id']
        ds_lock = {
            'id': ds['id'],
            'kind': ds['kind'],
            'family': ds.get('family'),
            'version': ds.get('version'),
            'files': []
        }
        if ds.get('kind') == 'existing':
            base = root / ds.get('root', '')
            for item in ds.get('files', []):
                p = base / item['path']
                if not p.exists():
                    raise RuntimeError('missing existing corpus file: %s' % p)
                verify_hash(p, item.get('expected_sha256'), 'existing')
                verify_size(p, item.get('expected_size'))
                ds_lock['files'].append({
                    'path': str(p),
                    'relative_path': item['path'],
                    'size': p.stat().st_size,
                    'sha256': sha256_file(p)
                })
        elif ds.get('kind') == 'files':
            for item in ds.get('files', []):
                out_path = ds_root / item['output']
                used = download_first(item['urls'], out_path, args.force)
                verify_hash(out_path, item.get('expected_sha256'), 'file')
                verify_size(out_path, item.get('expected_size'))
                ds_lock['files'].append({
                    'path': str(out_path),
                    'relative_path': item['output'],
                    'size': out_path.stat().st_size,
                    'sha256': sha256_file(out_path),
                    'source_url': used,
                })
        elif ds.get('kind') == 'archive':
            archive_meta = ds['archive']
            first_url = archive_meta['urls'][0]
            fname = first_url.rsplit('/', 1)[-1]
            archive_path = downloads / fname
            used = download_first(archive_meta['urls'], archive_path, args.force)
            verify_hash(archive_path, archive_meta.get('expected_sha256'), 'archive')
            verify_size(archive_path, archive_meta.get('expected_size'))
            ds_lock['archive'] = {
                'path': str(archive_path),
                'size': archive_path.stat().st_size,
                'sha256': sha256_file(archive_path),
                'source_url': used,
            }
            for item in ds.get('members', []):
                out_path = ds_root / item['output']
                fmt = archive_meta['format']
                if fmt == 'zip':
                    extract_zip_member(archive_path, item['member_suffix'], out_path)
                else:
                    extract_tar_member(archive_path, item['member_suffix'], out_path)
                verify_hash(out_path, item.get('expected_sha256'), 'member')
                verify_size(out_path, item.get('expected_size'))
                ds_lock['files'].append({
                    'path': str(out_path),
                    'relative_path': item['output'],
                    'member_suffix': item['member_suffix'],
                    'size': out_path.stat().st_size,
                    'sha256': sha256_file(out_path)
                })
        else:
            raise RuntimeError('unknown dataset kind: %s' % ds.get('kind'))
        lock['datasets'].append(ds_lock)

    lock_out = Path(args.lock_out) if args.lock_out else (root / 'phase69_corpus_lock.json')
    lock_out.parent.mkdir(parents=True, exist_ok=True)
    lock_out.write_text(json.dumps(lock, indent=2, sort_keys=False) + '\n', encoding='utf-8')
    print(lock_out)

    if args.freeze_manifest_out:
        frozen = build_frozen_manifest(manifest, lock)
        out = Path(args.freeze_manifest_out)
        out.parent.mkdir(parents=True, exist_ok=True)
        out.write_text(json.dumps(frozen, indent=2, sort_keys=False) + '\n', encoding='utf-8')
        print(out)

    return 0


if __name__ == '__main__':
    sys.exit(main())
