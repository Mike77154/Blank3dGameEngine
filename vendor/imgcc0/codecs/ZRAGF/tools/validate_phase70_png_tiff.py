#!/usr/bin/env python3
import argparse
import hashlib
import json
import struct
import subprocess
import sys
import zlib
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
            frames = []
            try:
                i = 0
                while True:
                    im.seek(i)
                    frames.append(im.tobytes())
                    i += 1
            except EOFError:
                pass
            return {
                'ok': True,
                'format': im.format,
                'width': int(width),
                'height': int(height),
                'frames': int(frame_count),
                'mode': mode,
                'pixel_sha256': sha256_bytes(b''.join(frames)) if frames else None,
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
            return {
                'ok': True,
                'pages': int(pages),
                'width': int(first.imagewidth),
                'height': int(first.imagelength),
                'dtype': str(arr.dtype),
                'shape': list(arr.shape),
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


def inspect_file_type(path: Path):
    try:
        proc = subprocess.run(
            ['file', '--brief', '--mime-type', str(path)],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            check=False,
        )
    except Exception as exc:
        return {'ok': False, 'error': str(exc)}
    if proc.returncode != 0:
        return {'ok': False, 'error': proc.stderr.strip() or proc.stdout.strip() or 'file failed'}
    return {'ok': True, 'mime': proc.stdout.strip()}


def inspect_png_chunks(path: Path):
    try:
        data = path.read_bytes()
    except Exception as exc:
        return {'ok': False, 'error': str(exc)}
    if len(data) < 8 or data[:8] != b'\x89PNG\r\n\x1a\n':
        return {'ok': False, 'error': 'bad PNG signature'}
    pos = 8
    seen_ihdr = 0
    seen_iend = 0
    width = None
    height = None
    chunks = []
    try:
        while pos + 12 <= len(data):
            length = struct.unpack('>I', data[pos:pos+4])[0]
            ctype = data[pos+4:pos+8]
            cdata_start = pos + 8
            cdata_end = cdata_start + length
            crc_pos = cdata_end
            if crc_pos + 4 > len(data):
                return {'ok': False, 'error': 'truncated chunk %r' % ctype}
            cdata = data[cdata_start:cdata_end]
            want_crc = struct.unpack('>I', data[crc_pos:crc_pos+4])[0]
            got_crc = zlib.crc32(ctype)
            got_crc = zlib.crc32(cdata, got_crc) & 0xffffffff
            if want_crc != got_crc:
                return {'ok': False, 'error': 'crc mismatch in chunk %r' % ctype}
            name = ctype.decode('ascii', errors='replace')
            chunks.append(name)
            if ctype == b'IHDR':
                seen_ihdr += 1
                if length >= 8:
                    width, height = struct.unpack('>II', cdata[:8])
            if ctype == b'IEND':
                seen_iend += 1
                pos = crc_pos + 4
                break
            pos = crc_pos + 4
        if seen_ihdr != 1 or seen_iend != 1:
            return {'ok': False, 'error': 'missing IHDR/IEND'}
        if pos != len(data):
            return {'ok': False, 'error': 'trailing bytes after IEND'}
        return {
            'ok': True,
            'width': int(width) if width is not None else None,
            'height': int(height) if height is not None else None,
            'frames': 1,
            'chunk_count': len(chunks),
            'chunks': chunks,
        }
    except Exception as exc:
        return {'ok': False, 'error': str(exc)}


def inspect_tiff_header(path: Path):
    try:
        data = path.read_bytes()
    except Exception as exc:
        return {'ok': False, 'error': str(exc)}
    if len(data) < 8:
        return {'ok': False, 'error': 'file too small'}
    bo = data[:2]
    if bo == b'II':
        endian = '<'
    elif bo == b'MM':
        endian = '>'
    else:
        return {'ok': False, 'error': 'bad byte order'}
    magic = struct.unpack(endian + 'H', data[2:4])[0]
    if magic == 42:
        ifd0 = struct.unpack(endian + 'I', data[4:8])[0]
        if ifd0 < 8 or ifd0 >= len(data):
            return {'ok': False, 'error': 'classic TIFF IFD offset out of range'}
        return {'ok': True, 'byte_order': bo.decode('ascii'), 'variant': 'classic', 'first_ifd': int(ifd0)}
    if magic == 43:
        if len(data) < 16:
            return {'ok': False, 'error': 'BigTIFF header truncated'}
        offsize = struct.unpack(endian + 'H', data[4:6])[0]
        zero = struct.unpack(endian + 'H', data[6:8])[0]
        ifd0 = struct.unpack(endian + 'Q', data[8:16])[0]
        if offsize != 8 or zero != 0 or ifd0 < 16 or ifd0 >= len(data):
            return {'ok': False, 'error': 'BigTIFF header invalid'}
        return {'ok': True, 'byte_order': bo.decode('ascii'), 'variant': 'bigtiff', 'first_ifd': int(ifd0)}
    return {'ok': False, 'error': 'bad TIFF magic %d' % magic}


def compare_dimensions(refs):
    dims = []
    for entry in refs:
        if entry.get('ok'):
            width = entry.get('width')
            height = entry.get('height')
            frames = entry.get('frames') or entry.get('pages')
            if width is not None and height is not None:
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
    readers = {'file': inspect_file_type(path)}
    structural = None
    if suffix == '.png':
        structural = inspect_png_chunks(path)
        readers['pillow'] = inspect_pillow(path)
        readers['identify'] = inspect_identify(path)
    elif suffix in ('.tif', '.tiff'):
        structural = inspect_tiff_header(path)
        readers['pillow'] = inspect_pillow(path)
        readers['tifffile'] = inspect_tifffile(path)
        readers['identify'] = inspect_identify(path)
    else:
        return None
    ok_dims, reason = compare_dimensions([readers[k] for k in readers if k != 'file'] + ([structural] if structural else []))
    mime_ok = readers['file'].get('ok') and ((suffix == '.png' and readers['file'].get('mime') == 'image/png') or (suffix in ('.tif', '.tiff') and readers['file'].get('mime') in ('image/tiff', 'image/x-tiff')))
    ok = ok_dims and mime_ok and structural is not None and structural.get('ok', False)
    if not ok:
        if structural is not None and not structural.get('ok', False):
            reason = structural.get('error', reason)
        elif not mime_ok:
            reason = 'unexpected mime type %r' % readers['file'].get('mime')
    return {
        'path': str(path),
        'sha256': sha256_file(path),
        'suffix': suffix,
        'ok': ok,
        'reason': reason,
        'structural': structural,
        'readers': readers,
    }


def main() -> int:
    ap = argparse.ArgumentParser(description='Differentially validate PNG/TIFF corpora with multiple local readers and structural parsers.')
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
        'phase': 70,
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
