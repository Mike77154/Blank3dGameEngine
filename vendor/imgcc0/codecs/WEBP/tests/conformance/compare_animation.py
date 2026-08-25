#!/usr/bin/env python3
import argparse
import json
from pathlib import Path


def read_jsonl(path: Path):
    items = []
    if not path.exists():
        raise SystemExit(f'no existe {path}')
    for raw in path.read_text(encoding='utf-8').splitlines():
        line = raw.strip()
        if not line:
            continue
        items.append(json.loads(line))
    return items


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


def calc_stats(lhs: bytes, rhs: bytes):
    total = 0
    max_diff = 0
    first = None
    for i, (a, b) in enumerate(zip(lhs, rhs)):
        if a != b and first is None:
            first = {'offset': i, 'lhs': a, 'rhs': b}
        d = abs(a - b)
        total += d
        if d > max_diff:
            max_diff = d
    mean = float(total) / float(len(lhs)) if lhs else 0.0
    return mean, max_diff, first


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('lhs_jsonl')
    ap.add_argument('rhs_jsonl')
    ap.add_argument('--lhs-prefix', required=True)
    ap.add_argument('--rhs-prefix', required=True)
    ap.add_argument('--mean-threshold', type=float, default=0.0)
    ap.add_argument('--max-threshold', type=int, default=0)
    args = ap.parse_args()

    lhs_meta = read_jsonl(Path(args.lhs_jsonl))
    rhs_meta = read_jsonl(Path(args.rhs_jsonl))
    if len(lhs_meta) != len(rhs_meta):
        print(json.dumps({'status': 'FAIL', 'reason': 'frame-count-mismatch', 'lhs_count': len(lhs_meta), 'rhs_count': len(rhs_meta)}, indent=2, sort_keys=True))
        return

    frames = []
    overall_status = 'PASS_EXACT'
    for i, (lmeta, rmeta) in enumerate(zip(lhs_meta, rhs_meta)):
      lpath = Path(f'{args.lhs_prefix}_{i:03d}.pam')
      rpath = Path(f'{args.rhs_prefix}_{i:03d}.pam')
      la = read_pam(lpath)
      ra = read_pam(rpath)
      if (la['width'], la['height'], la['depth']) != (ra['width'], ra['height'], ra['depth']):
          frames.append({'index': i, 'status': 'FAIL', 'reason': 'shape-mismatch'})
          overall_status = 'FAIL'
          continue
      mean, max_diff, first = calc_stats(la['payload'], ra['payload'])
      if la['payload'] == ra['payload']:
          status = 'PASS_EXACT'
      elif mean <= args.mean_threshold and max_diff <= args.max_threshold:
          status = 'PASS_VISUAL_BUT_NOT_EXACT'
          if overall_status == 'PASS_EXACT':
              overall_status = status
      else:
          status = 'FAIL'
          overall_status = 'FAIL'
      meta_same = (
          lmeta.get('timestamp_ms') == rmeta.get('timestamp_ms') and
          lmeta.get('duration_ms') == rmeta.get('duration_ms')
      )
      if not meta_same and status != 'FAIL':
          status = 'FAIL'
          overall_status = 'FAIL'
      frames.append({
          'index': i,
          'status': status,
          'mean_abs_diff': mean,
          'max_abs_diff': max_diff,
          'first_mismatch': first,
          'lhs_meta': lmeta,
          'rhs_meta': rmeta,
      })
    print(json.dumps({'status': overall_status, 'frame_count': len(frames), 'frames': frames}, indent=2, sort_keys=True))


if __name__ == '__main__':
    main()
