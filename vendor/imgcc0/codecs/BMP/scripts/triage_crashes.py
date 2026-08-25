#!/usr/bin/env python3
import argparse
import hashlib
import html
import json
import os
import re
import shutil
import subprocess
import sys
from collections import defaultdict
from pathlib import Path

ASAN_RE = re.compile(r"ERROR: AddressSanitizer: ([^\n]+)")
UBSAN_RE = re.compile(r"UndefinedBehaviorSanitizer: ([^\n]+)")
UBSAN_RUNTIME_RE = re.compile(r"runtime error: ([^\n]+)")
SUMMARY_RE = re.compile(r"SUMMARY: ([^\n]+)")
FRAME_RE = re.compile(r"^\s*#\d+\s+(?:0x[0-9a-fA-F]+\s+)?in\s+([^\s(]+)", re.MULTILINE)
HEX_RE = re.compile(r"0x[0-9a-fA-F]+")
NUM_RE = re.compile(r"\b\d+\b")
WHITESPACE_RE = re.compile(r"\s+")

BORING_FRAMES = {
    'main', '_start', '__libc_start_main', 'start_thread', 'LLVMFuzzerTestOneInput',
    '__sanitizer', '__asan', '__ubsan', 'abort', 'raise', 'memcpy', 'memmove',
}


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open('rb') as f:
        for chunk in iter(lambda: f.read(65536), b''):
            h.update(chunk)
    return h.hexdigest()


def normalize_text(value: str) -> str:
    value = HEX_RE.sub('0xADDR', value)
    value = NUM_RE.sub('N', value)
    value = WHITESPACE_RE.sub(' ', value.strip())
    return value.lower()


def detect_signature(text: str):
    for kind, rx in (
        ('asan', ASAN_RE),
        ('ubsan', UBSAN_RE),
        ('ubsan-runtime', UBSAN_RUNTIME_RE),
        ('summary', SUMMARY_RE),
    ):
        m = rx.search(text)
        if m:
            raw = m.group(1).strip()
            return kind, raw, normalize_text(raw)
    if 'timeout' in text.lower():
        return 'timeout', 'timeout', 'timeout'
    return 'unknown', 'no-crash-signature', 'no-crash-signature'


def extract_frames(text: str, limit: int = 4):
    frames = []
    for match in FRAME_RE.finditer(text):
        fn = match.group(1).strip()
        low = fn.lower()
        if not fn:
            continue
        if any(token in low for token in BORING_FRAMES):
            continue
        if low.startswith('__interceptor_'):
            continue
        frames.append(fn)
        if len(frames) >= limit:
            break
    return frames


def bucketize(kind: str, signature_norm: str, frames):
    parts = [kind, signature_norm] + list(frames[:3])
    material = '|'.join(parts) if parts else 'empty'
    bucket_id = hashlib.sha1(material.encode('utf-8')).hexdigest()[:12]
    title_bits = []
    if kind and kind != 'unknown':
        title_bits.append(kind)
    if signature_norm and signature_norm != 'no-crash-signature':
        title_bits.append(signature_norm)
    if frames:
        title_bits.append(' > '.join(frames[:2]))
    title = ' | '.join(title_bits) if title_bits else 'unclassified'
    return bucket_id, title


def run_case(mode: str, harness: str, testcase: Path, timeout: int):
    env = os.environ.copy()
    env.setdefault('ASAN_OPTIONS', 'symbolize=1:abort_on_error=1:detect_leaks=0')
    env.setdefault('UBSAN_OPTIONS', 'print_stacktrace=1:halt_on_error=1')
    env.setdefault('ASAN_SYMBOLIZER_PATH', shutil.which('llvm-symbolizer') or '')

    if mode == 'libfuzzer':
        cmd = [harness, '-runs=1', str(testcase)]
    elif mode == 'stdin':
        cmd = [harness]
    else:
        cmd = [harness, str(testcase)]

    try:
        if mode == 'stdin':
            data = testcase.read_bytes()
            proc = subprocess.run(cmd, input=data, capture_output=True, timeout=timeout, env=env)
        else:
            proc = subprocess.run(cmd, capture_output=True, timeout=timeout, env=env)
        status = 'crash' if proc.returncode != 0 else 'ok'
        out = (proc.stdout or b'').decode('utf-8', 'replace')
        err = (proc.stderr or b'').decode('utf-8', 'replace')
        return {
            'status': status,
            'exit_code': proc.returncode,
            'stdout': out,
            'stderr': err,
            'timed_out': False,
            'command': cmd,
        }
    except subprocess.TimeoutExpired as exc:
        out = (exc.stdout or b'').decode('utf-8', 'replace') if exc.stdout else ''
        err = (exc.stderr or b'').decode('utf-8', 'replace') if exc.stderr else ''
        return {
            'status': 'timeout',
            'exit_code': None,
            'stdout': out,
            'stderr': err,
            'timed_out': True,
            'command': cmd,
        }


def minimize_case(mode: str, harness: str, testcase: Path, output_dir: Path, timeout: int):
    output_dir.mkdir(parents=True, exist_ok=True)
    out_path = output_dir / (testcase.name + '.min')

    if mode == 'libfuzzer':
        cmd = [
            harness,
            '-runs=100000',
            '-minimize_crash=1',
            f'-exact_artifact_path={out_path}',
            str(testcase),
        ]
        env = os.environ.copy()
        env.setdefault('ASAN_OPTIONS', 'symbolize=1:abort_on_error=1:detect_leaks=0')
        env.setdefault('UBSAN_OPTIONS', 'print_stacktrace=1:halt_on_error=1')
        env.setdefault('ASAN_SYMBOLIZER_PATH', shutil.which('llvm-symbolizer') or '')
        try:
            proc = subprocess.run(cmd, capture_output=True, timeout=timeout, env=env)
            created = out_path.exists()
            return {
                'attempted': True,
                'method': 'libfuzzer-minimize_crash',
                'output': str(out_path) if created else None,
                'exit_code': proc.returncode,
            }
        except subprocess.TimeoutExpired:
            return {
                'attempted': True,
                'method': 'libfuzzer-minimize_crash',
                'output': None,
                'exit_code': None,
                'timed_out': True,
            }

    afl_tmin = shutil.which('afl-tmin')
    if afl_tmin is None:
        return {
            'attempted': False,
            'method': 'afl-tmin',
            'reason': 'afl-tmin not found',
            'output': None,
        }

    cmd = [afl_tmin, '-i', str(testcase), '-o', str(out_path), '--', harness, '@@']
    try:
        proc = subprocess.run(cmd, capture_output=True, timeout=timeout)
        created = out_path.exists()
        return {
            'attempted': True,
            'method': 'afl-tmin',
            'output': str(out_path) if created else None,
            'exit_code': proc.returncode,
        }
    except subprocess.TimeoutExpired:
        return {
            'attempted': True,
            'method': 'afl-tmin',
            'output': None,
            'exit_code': None,
            'timed_out': True,
        }


def rel_or_none(path: Path, root: Path):
    if path is None:
        return None
    try:
        return str(path.relative_to(root))
    except Exception:
        return str(path)


def write_html(output_dir: Path, summary: dict):
    rows = []
    for bucket in summary['buckets']:
        rep = html.escape(bucket.get('representative', ''))
        sig = html.escape(bucket.get('signature_raw', ''))
        frames = '<br>'.join(html.escape(frame) for frame in bucket.get('top_frames', [])) or '&nbsp;'
        rows.append(
            '<tr>'
            f'<td><code>{html.escape(bucket["bucket_id"])}</code></td>'
            f'<td>{bucket["count"]}</td>'
            f'<td>{html.escape(bucket["kind"])}</td>'
            f'<td>{sig}</td>'
            f'<td>{frames}</td>'
            f'<td>{rep}</td>'
            '</tr>'
        )

    case_rows = []
    for case in summary['cases']:
        case_rows.append(
            '<tr>'
            f'<td>{html.escape(case["file"])}</td>'
            f'<td>{html.escape(case["status"])}</td>'
            f'<td>{html.escape(case["bucket_id"] or "-")}</td>'
            f'<td>{html.escape(case["signature_raw"])}</td>'
            f'<td>{html.escape(", ".join(case.get("top_frames", [])))}</td>'
            '</tr>'
        )

    html_doc = f'''<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <title>Crash triage report</title>
  <style>
    body {{ font-family: system-ui, sans-serif; margin: 2rem; color: #222; }}
    table {{ border-collapse: collapse; width: 100%; margin: 1rem 0 2rem; }}
    th, td {{ border: 1px solid #ddd; padding: 0.5rem; text-align: left; vertical-align: top; }}
    th {{ background: #f6f6f6; }}
    code {{ font-family: ui-monospace, SFMono-Regular, monospace; }}
    .meta {{ display: grid; grid-template-columns: repeat(auto-fit, minmax(220px, 1fr)); gap: 0.5rem 1rem; }}
    .card {{ border: 1px solid #ddd; padding: 0.75rem 1rem; border-radius: 8px; background: #fafafa; }}
  </style>
</head>
<body>
  <h1>Crash triage report</h1>
  <div class="meta">
    <div class="card"><strong>Mode</strong><br>{html.escape(summary['mode'])}</div>
    <div class="card"><strong>Harness</strong><br><code>{html.escape(summary['harness'])}</code></div>
    <div class="card"><strong>Total cases</strong><br>{summary['cases_total']}</div>
    <div class="card"><strong>Crash buckets</strong><br>{summary['buckets_total']}</div>
    <div class="card"><strong>Crashes</strong><br>{summary['cases_crash']}</div>
    <div class="card"><strong>Timeouts</strong><br>{summary['cases_timeout']}</div>
  </div>

  <h2>Buckets</h2>
  <table>
    <thead><tr><th>Bucket</th><th>Count</th><th>Kind</th><th>Signature</th><th>Top frames</th><th>Representative</th></tr></thead>
    <tbody>
      {''.join(rows) if rows else '<tr><td colspan="6">No crash buckets found.</td></tr>'}
    </tbody>
  </table>

  <h2>Cases</h2>
  <table>
    <thead><tr><th>File</th><th>Status</th><th>Bucket</th><th>Signature</th><th>Top frames</th></tr></thead>
    <tbody>
      {''.join(case_rows) if case_rows else '<tr><td colspan="5">No cases found.</td></tr>'}
    </tbody>
  </table>
</body>
</html>
'''
    (output_dir / 'report.html').write_text(html_doc, encoding='utf-8')


def main() -> int:
    ap = argparse.ArgumentParser(description='Round 7 crash triage helper with bucketing')
    ap.add_argument('--mode', choices=['libfuzzer', 'file', 'stdin'], required=True)
    ap.add_argument('--harness', required=True)
    ap.add_argument('--input-dir', required=True)
    ap.add_argument('--output-dir', required=True)
    ap.add_argument('--timeout', type=int, default=10)
    ap.add_argument('--minimize', action='store_true')
    args = ap.parse_args()

    harness = args.harness
    input_dir = Path(args.input_dir)
    output_dir = Path(args.output_dir)
    if not input_dir.is_dir():
        print(f'error: input directory not found: {input_dir}', file=sys.stderr)
        return 1
    output_dir.mkdir(parents=True, exist_ok=True)
    minimized_dir = output_dir / 'minimized'
    cases_dir = output_dir / 'cases'
    buckets_root = output_dir / 'buckets'
    cases_dir.mkdir(parents=True, exist_ok=True)
    buckets_root.mkdir(parents=True, exist_ok=True)

    cases = []
    bucket_members = defaultdict(list)

    for testcase in sorted(p for p in input_dir.iterdir() if p.is_file()):
        if testcase.name.startswith('.') or testcase.name.lower().startswith('readme'):
            continue
        digest = sha256_file(testcase)
        run = run_case(args.mode, harness, testcase, args.timeout)
        combined = (run['stdout'] or '') + '\n' + (run['stderr'] or '')
        kind, signature_raw, signature_norm = detect_signature(combined)
        top_frames = extract_frames(combined)
        bucket_id = None
        bucket_title = None
        if run['status'] != 'ok':
            bucket_id, bucket_title = bucketize(kind, signature_norm, top_frames)
            bucket_members[bucket_id].append(testcase)

        case_dir = cases_dir / digest[:16]
        case_dir.mkdir(parents=True, exist_ok=True)
        stored_input = case_dir / testcase.name
        shutil.copy2(testcase, stored_input)
        (case_dir / 'stdout.txt').write_text(run['stdout'], encoding='utf-8')
        (case_dir / 'stderr.txt').write_text(run['stderr'], encoding='utf-8')

        case = {
            'file': testcase.name,
            'sha256': digest,
            'size': testcase.stat().st_size,
            'status': run['status'],
            'exit_code': run['exit_code'],
            'signature_kind': kind,
            'signature_raw': signature_raw,
            'signature_normalized': signature_norm,
            'top_frames': top_frames,
            'bucket_id': bucket_id,
            'bucket_title': bucket_title,
            'timed_out': run['timed_out'],
            'command': run['command'],
            'artifacts': {
                'input': rel_or_none(stored_input, output_dir),
                'stdout': rel_or_none(case_dir / 'stdout.txt', output_dir),
                'stderr': rel_or_none(case_dir / 'stderr.txt', output_dir),
            },
        }
        if args.minimize and case['status'] != 'ok':
            case['minimize'] = minimize_case(args.mode, harness, testcase, minimized_dir, args.timeout)
            if case['minimize'].get('output'):
                min_path = Path(case['minimize']['output'])
                case['minimize']['output'] = rel_or_none(min_path, output_dir)
        cases.append(case)

    buckets = []
    for bucket_id, members in sorted(bucket_members.items(), key=lambda item: (-len(item[1]), item[0])):
        bucket_cases = [c for c in cases if c['bucket_id'] == bucket_id]
        rep = bucket_cases[0]
        bucket_dir = buckets_root / bucket_id
        bucket_dir.mkdir(parents=True, exist_ok=True)
        inputs_dir = bucket_dir / 'inputs'
        inputs_dir.mkdir(parents=True, exist_ok=True)
        for member in members:
            shutil.copy2(member, inputs_dir / member.name)
        bucket_info = {
            'bucket_id': bucket_id,
            'title': rep['bucket_title'],
            'kind': rep['signature_kind'],
            'signature_raw': rep['signature_raw'],
            'signature_normalized': rep['signature_normalized'],
            'top_frames': rep['top_frames'],
            'count': len(bucket_cases),
            'representative': rep['file'],
            'cases': [c['file'] for c in bucket_cases],
            'paths': {
                'dir': rel_or_none(bucket_dir, output_dir),
                'inputs': rel_or_none(inputs_dir, output_dir),
            },
        }
        (bucket_dir / 'bucket.json').write_text(json.dumps(bucket_info, indent=2, sort_keys=True), encoding='utf-8')
        buckets.append(bucket_info)

    summary = {
        'mode': args.mode,
        'harness': harness,
        'input_dir': str(input_dir),
        'output_dir': str(output_dir),
        'cases_total': len(cases),
        'cases_crash': sum(1 for c in cases if c['status'] == 'crash'),
        'cases_timeout': sum(1 for c in cases if c['status'] == 'timeout'),
        'cases_ok': sum(1 for c in cases if c['status'] == 'ok'),
        'buckets_total': len(buckets),
        'buckets': buckets,
        'cases': cases,
    }

    (output_dir / 'report.json').write_text(json.dumps(summary, indent=2, sort_keys=True), encoding='utf-8')
    (output_dir / 'buckets.json').write_text(json.dumps({'buckets': buckets}, indent=2, sort_keys=True), encoding='utf-8')
    with (output_dir / 'summary.txt').open('w', encoding='utf-8') as f:
        f.write(f"mode: {summary['mode']}\n")
        f.write(f"harness: {summary['harness']}\n")
        f.write(f"cases_total: {summary['cases_total']}\n")
        f.write(f"cases_crash: {summary['cases_crash']}\n")
        f.write(f"cases_timeout: {summary['cases_timeout']}\n")
        f.write(f"cases_ok: {summary['cases_ok']}\n")
        f.write(f"buckets_total: {summary['buckets_total']}\n\n")
        for bucket in buckets:
            f.write(f"[{bucket['bucket_id']}] count={bucket['count']} kind={bucket['kind']} sig={bucket['signature_raw']}\n")
            if bucket['top_frames']:
                f.write(f"  frames: {' > '.join(bucket['top_frames'])}\n")
            for name in bucket['cases']:
                f.write(f"  - {name}\n")
        if not buckets:
            f.write('No crash buckets found.\n')

    write_html(output_dir, summary)
    print(f"triage report written to {output_dir / 'report.json'}")
    print(f"bucket summary written to {output_dir / 'buckets.json'}")
    print(f"html report written to {output_dir / 'report.html'}")
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
