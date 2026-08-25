#!/usr/bin/env python3
import json
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PYTHON = sys.executable or 'python3'
TRIAGE = ROOT / 'scripts' / 'triage_crashes.py'
BUNDLE = ROOT / 'scripts' / 'bundle_ci_artifacts.sh'
MOCK = ROOT / 'tests' / 'mock_crash_harness.py'


def run(cmd, **kwargs):
    print('+', ' '.join(str(x) for x in cmd))
    return subprocess.run(cmd, check=True, **kwargs)


def main() -> int:
    tmp = Path(tempfile.mkdtemp(prefix='round7-tools-'))
    try:
        crash_dir = tmp / 'crashes'
        crash_dir.mkdir()
        (crash_dir / 'a_over').write_bytes(b'OVER one')
        (crash_dir / 'b_over').write_bytes(b'OVER two')
        (crash_dir / 'c_null').write_bytes(b'NULL here')
        (crash_dir / 'd_ubsn').write_bytes(b'UBSN here')
        (crash_dir / 'e_ok').write_bytes(b'OK')

        triage_out = tmp / 'triage'
        run([PYTHON, str(TRIAGE), '--mode', 'file', '--harness', str(MOCK), '--input-dir', str(crash_dir), '--output-dir', str(triage_out), '--timeout', '2'])
        report = json.loads((triage_out / 'report.json').read_text(encoding='utf-8'))
        buckets = json.loads((triage_out / 'buckets.json').read_text(encoding='utf-8'))

        assert report['cases_total'] == 5, report
        assert report['cases_ok'] == 1, report
        assert report['cases_crash'] == 4, report
        assert report['buckets_total'] == 3, report
        bucket_counts = sorted(bucket['count'] for bucket in buckets['buckets'])
        assert bucket_counts == [1, 1, 2], bucket_counts
        assert (triage_out / 'report.html').is_file()

        out_dir = tmp / 'bundle'
        log_file = tmp / 'mock.log'
        log_file.write_text('mock log\n', encoding='utf-8')
        env = os.environ.copy()
        env['PYTHON'] = PYTHON
        run(['sh', str(BUNDLE), str(out_dir), 'demo', 'mock', str(MOCK), str(crash_dir), '', str(log_file), str(triage_out)], env=env)

        manifest = json.loads((out_dir / 'manifest.json').read_text(encoding='utf-8'))
        index = json.loads((out_dir / 'index.json').read_text(encoding='utf-8'))
        assert manifest['project'] == 'demo'
        assert manifest['target'] == 'mock'
        assert manifest['triage_reports'], manifest
        assert (out_dir / 'index.html').is_file()
        assert (out_dir / 'summary.md').is_file()
        assert index['files_total'] >= 1
        print('round7 tools self-test: OK')
        return 0
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


if __name__ == '__main__':
    raise SystemExit(main())
