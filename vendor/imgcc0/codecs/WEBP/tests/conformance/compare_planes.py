#!/usr/bin/env python3
import argparse
import hashlib
import json
from pathlib import Path


def md5_bytes(data: bytes) -> str:
    return hashlib.md5(data).hexdigest()


def read_pam(path: Path):
    with path.open('rb') as f:
        header = []
        while True:
            line = f.readline()
            if not line:
                raise ValueError('PAM truncado')
            header.append(line)
            if line.strip() == b'ENDHDR':
                break
        payload = f.read()
    width = height = depth = None
    for raw in header:
        parts = raw.decode('ascii', 'ignore').strip().split()
        if len(parts) == 2 and parts[0] == 'WIDTH':
            width = int(parts[1])
        elif len(parts) == 2 and parts[0] == 'HEIGHT':
            height = int(parts[1])
        elif len(parts) == 2 and parts[0] == 'DEPTH':
            depth = int(parts[1])
    if width is None or height is None or depth is None:
        raise ValueError('PAM sin WIDTH/HEIGHT/DEPTH')
    expected = width * height * depth
    if len(payload) != expected:
        raise ValueError(f'payload PAM inesperado: {len(payload)} != {expected}')
    return {'width': width, 'height': height, 'depth': depth, 'payload': payload}


def read_yuv420(path: Path, width: int, height: int):
    payload = path.read_bytes()
    uv_w = (width + 1) // 2
    uv_h = (height + 1) // 2
    expected = width * height + 2 * uv_w * uv_h
    if len(payload) != expected:
        raise ValueError(f'payload YUV inesperado: {len(payload)} != {expected}')
    y_end = width * height
    u_end = y_end + uv_w * uv_h
    return {
        'width': width,
        'height': height,
        'depth': 3,
        'payload': payload,
        'planes': [payload[:y_end], payload[y_end:u_end], payload[u_end:]],
        'plane_names': ['Y', 'U', 'V'],
    }


def pam_planes(obj):
    depth = obj['depth']
    payload = obj['payload']
    planes = [bytearray() for _ in range(depth)]
    for i, b in enumerate(payload):
        planes[i % depth].append(b)
    names = ['R', 'G', 'B', 'A'][:depth]
    return [bytes(p) for p in planes], names


def compare_buffers(lhs: bytes, rhs: bytes, name: str):
    if len(lhs) != len(rhs):
        return {'plane': name, 'offset': 0, 'lhs': len(lhs), 'rhs': len(rhs), 'type': 'size'}
    for i, (a, b) in enumerate(zip(lhs, rhs)):
        if a != b:
            return {'plane': name, 'offset': i, 'lhs': a, 'rhs': b, 'type': 'byte'}
    return None


def calc_stats(lhs: bytes, rhs: bytes):
    total = 0
    max_diff = 0
    for a, b in zip(lhs, rhs):
        d = abs(a - b)
        total += d
        if d > max_diff:
            max_diff = d
    mean = float(total) / float(len(lhs)) if lhs else 0.0
    return mean, max_diff


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--format', choices=['pam', 'yuv420p'], required=True)
    ap.add_argument('lhs')
    ap.add_argument('rhs')
    ap.add_argument('--width', type=int)
    ap.add_argument('--height', type=int)
    ap.add_argument('--mean-threshold', type=float, default=0.0)
    ap.add_argument('--max-threshold', type=int, default=0)
    args = ap.parse_args()

    if args.format == 'pam':
        a = read_pam(Path(args.lhs))
        b = read_pam(Path(args.rhs))
        if (a['width'], a['height'], a['depth']) != (b['width'], b['height'], b['depth']):
            result = {'status': 'FAIL', 'reason': 'shape-mismatch', 'lhs': [a['width'], a['height'], a['depth']], 'rhs': [b['width'], b['height'], b['depth']]}
            print(json.dumps(result, indent=2, sort_keys=True))
            return
        a_planes, names = pam_planes(a)
        b_planes, _ = pam_planes(b)
        payload_a = a['payload']
        payload_b = b['payload']
        shape = [a['width'], a['height'], a['depth']]
    else:
        if args.width is None or args.height is None:
            raise SystemExit('--width y --height son obligatorios para yuv420p')
        a = read_yuv420(Path(args.lhs), args.width, args.height)
        b = read_yuv420(Path(args.rhs), args.width, args.height)
        a_planes = a['planes']
        b_planes = b['planes']
        names = a['plane_names']
        payload_a = a['payload']
        payload_b = b['payload']
        shape = [args.width, args.height, 3]

    first = None
    per_plane = {}
    exact = payload_a == payload_b
    overall_mean, overall_max = calc_stats(payload_a, payload_b)
    for name, pa, pb in zip(names, a_planes, b_planes):
        mismatch = compare_buffers(pa, pb, name)
        mean, max_diff = calc_stats(pa, pb)
        per_plane[name] = {'mean_abs_diff': mean, 'max_abs_diff': max_diff, 'md5_lhs': md5_bytes(pa), 'md5_rhs': md5_bytes(pb)}
        if first is None and mismatch is not None:
            first = mismatch
    if exact:
        status = 'PASS_EXACT'
    elif overall_mean <= args.mean_threshold and overall_max <= args.max_threshold:
        status = 'PASS_VISUAL_BUT_NOT_EXACT'
    else:
        status = 'FAIL'
    result = {
        'status': status,
        'shape': shape,
        'format': args.format,
        'md5_lhs': md5_bytes(payload_a),
        'md5_rhs': md5_bytes(payload_b),
        'mean_abs_diff': overall_mean,
        'max_abs_diff': overall_max,
        'first_mismatch': first,
        'per_plane': per_plane,
    }
    print(json.dumps(result, indent=2, sort_keys=True))


if __name__ == '__main__':
    main()
