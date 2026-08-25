#!/usr/bin/env python3
import argparse
import hashlib
import json
import subprocess
import sys
from pathlib import Path

try:
    from PIL import Image
except Exception:  # pragma: no cover
    Image = None

try:
    import tifffile
except Exception:  # pragma: no cover
    tifffile = None


def sha256_bytes(data: bytes) -> str:
    h = hashlib.sha256()
    h.update(data)
    return h.hexdigest()


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open('rb') as f:
        while True:
            chunk = f.read(65536)
            if not chunk:
                break
            h.update(chunk)
    return h.hexdigest()


def collect_files(manifest: dict, root: Path):
    out = []
    for ds in manifest.get('datasets', []):
        kind = ds.get('kind')
        if kind == 'existing':
            base = root / ds.get('root', '')
            for item in ds.get('files', []):
                out.append(base / item['path'])
        elif kind == 'files':
            base = root / ds['id']
            for item in ds.get('files', []):
                out.append(base / item['output'])
        elif kind == 'archive':
            base = root / ds['id']
            for item in ds.get('members', []):
                out.append(base / item['output'])
    return out


def inspect_pillow(path: Path):
    if Image is None:
        return {'ok': False, 'error': 'Pillow unavailable'}
    try:
        with Image.open(path) as im:
            frame_count = getattr(im, 'n_frames', 1)
            width, height = im.size
            mode = im.mode
            pixel_digest = None
            try:
                frames = []
                i = 0
                while True:
                    im.seek(i)
                    frames.append(im.tobytes())
                    i += 1
            except EOFError:
                pass
            if frames:
                pixel_digest = sha256_bytes(b''.join(frames))
            return {
                'ok': True,
                'format': im.format,
                'width': width,
                'height': height,
                'frames': frame_count,
                'mode': mode,
                'pixel_sha256': pixel_digest,
            }
    except Exception as exc:
        return {'ok': False, 'error': str(exc)}


def inspect_tifffile(path: Path):
    if tifffile is None:
        return {'ok': False, 'error': 'tifffile unavailable'}
    try:
        with tifffile.TiffFile(str(path)) as tf:
            pages = len(tf.pages)
            first = tf.pages[0]
            arr = tf.asarray()
            shape = list(arr.shape)
            return {
                'ok': True,
                'pages': pages,
                'width': int(first.imagewidth),
                'height': int(first.imagelength),
                'dtype': str(arr.dtype),
                'shape': shape,
                'pixel_sha256': sha256_bytes(arr.tobytes()),
            }
    except Exception as exc:
        return {'ok': False, 'error': str(exc)}


def inspect_identify(path: Path):
    try:
        proc = subprocess.run(
            ['identify', '-format', '%m %w %h %n', str(path)],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            check=False,
        )
    except Exception as exc:
        return {'ok': False, 'error': str(exc)}
    if proc.returncode != 0:
        return {'ok': False, 'error': proc.stderr.strip() or proc.stdout.strip() or 'identify failed'}
    parts = proc.stdout.strip().split()
    if len(parts) < 4:
        return {'ok': False, 'error': 'unexpected identify output: %r' % proc.stdout}
    try:
        return {
            'ok': True,
            'format': parts[0],
            'width': int(parts[1]),
            'height': int(parts[2]),
            'frames': int(parts[3]),
        }
    except Exception as exc:
        return {'ok': False, 'error': 'parse identify output failed: %s' % exc}


def compare_dimensions(refs):
    dims = []
    for entry in refs:
        if entry.get('ok'):
            width = entry.get('width')
            height = entry.get('height')
            frames = entry.get('frames') or entry.get('pages')
            dims.append((width, height, frames))
    if not dims:
        return False, 'no successful readers'
    first = dims[0]
    for item in dims[1:]:
        if item != first:
            return False, 'dimension/frame mismatch %r vs %r' % (first, item)
    return True, ''


def validate_path(path: Path):
    suffix = path.suffix.lower()
    readers = {}
    if suffix == '.png':
        readers['pillow'] = inspect_pillow(path)
        readers['identify'] = inspect_identify(path)
    elif suffix in ('.tif', '.tiff'):
        readers['pillow'] = inspect_pillow(path)
        readers['tifffile'] = inspect_tifffile(path)
        readers['identify'] = inspect_identify(path)
    else:
        return None
    ok, reason = compare_dimensions([readers[k] for k in readers])
    return {
        'path': str(path),
        'sha256': sha256_file(path),
        'suffix': suffix,
        'ok': ok,
        'reason': reason,
        'readers': readers,
    }


def main() -> int:
    ap = argparse.ArgumentParser(description='Differentially validate PNG/TIFF corpora with multiple local readers.')
    ap.add_argument('--manifest')
    ap.add_argument('--root', default='bench/corpora')
    ap.add_argument('--json-out')
    ap.add_argument('files', nargs='*')
    args = ap.parse_args()

    paths = []
    if args.manifest:
        manifest = json.loads(Path(args.manifest).read_text(encoding='utf-8'))
        paths.extend(collect_files(manifest, Path(args.root)))
    paths.extend(Path(p) for p in args.files)
    results = []
    failures = 0
    for path in paths:
        entry = validate_path(path)
        if entry is None:
            continue
        results.append(entry)
        if not entry['ok']:
            failures += 1
    payload = {
        'phase': 69,
        'count': len(results),
        'failures': failures,
        'results': results,
    }
    text = json.dumps(payload, indent=2, sort_keys=False) + '\n'
    if args.json_out:
        out = Path(args.json_out)
        out.parent.mkdir(parents=True, exist_ok=True)
        out.write_text(text, encoding='utf-8')
        print(out)
    else:
        sys.stdout.write(text)
    return 0 if failures == 0 else 1


if __name__ == '__main__':
    sys.exit(main())
