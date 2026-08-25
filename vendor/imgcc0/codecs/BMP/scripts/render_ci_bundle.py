#!/usr/bin/env python3
import argparse
import html
import json
from pathlib import Path


def rel(path: Path, root: Path) -> str:
    try:
        return str(path.relative_to(root))
    except Exception:
        return str(path)


def collect_files(root: Path):
    files = []
    for p in sorted(root.rglob('*')):
        if p.is_file():
            files.append({
                'path': rel(p, root),
                'size': p.stat().st_size,
                'kind': p.suffix.lower().lstrip('.') or 'file',
            })
    return files


def count_non_hidden(path: Path):
    if path is None or not path.is_dir():
        return 0
    total = 0
    for p in path.rglob('*'):
        if p.is_file() and not p.name.startswith('.') and not p.name.lower().startswith('readme'):
            total += 1
    return total


def parse_manifest_txt(path: Path):
    data = {}
    if not path.is_file():
        return data
    for line in path.read_text(encoding='utf-8', errors='replace').splitlines():
        if ':' not in line:
            continue
        key, value = line.split(':', 1)
        data[key.strip()] = value.strip()
    return data


def load_json_if_exists(path: Path):
    if not path.is_file():
        return None
    try:
        return json.loads(path.read_text(encoding='utf-8'))
    except Exception:
        return None


def write_html(out_dir: Path, index_data: dict):
    file_rows = ''.join(
        f'<tr><td><code>{html.escape(item["path"])}</code></td><td>{item["size"]}</td><td>{html.escape(item["kind"])}</td></tr>'
        for item in index_data['files']
    ) or '<tr><td colspan="3">No files.</td></tr>'

    triage_rows = ''.join(
        f'<tr><td><code>{html.escape(item["path"])}</code></td><td>{item.get("buckets_total", 0)}</td><td>{item.get("cases_crash", 0)}</td><td>{item.get("cases_timeout", 0)}</td></tr>'
        for item in index_data.get('triage_reports', [])
    ) or '<tr><td colspan="4">No triage reports found.</td></tr>'

    html_doc = f'''<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <title>CI artifact bundle</title>
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
  <h1>CI artifact bundle</h1>
  <div class="meta">
    <div class="card"><strong>Project</strong><br>{html.escape(index_data['project'])}</div>
    <div class="card"><strong>Target</strong><br>{html.escape(index_data['target'])}</div>
    <div class="card"><strong>Total files</strong><br>{index_data['files_total']}</div>
    <div class="card"><strong>Source corpus files</strong><br>{index_data['source_corpus_files']}</div>
    <div class="card"><strong>Minimized corpus files</strong><br>{index_data['minimized_corpus_files']}</div>
    <div class="card"><strong>Triage reports</strong><br>{len(index_data.get('triage_reports', []))}</div>
  </div>

  <h2>Bundle files</h2>
  <table>
    <thead><tr><th>Path</th><th>Size</th><th>Kind</th></tr></thead>
    <tbody>{file_rows}</tbody>
  </table>

  <h2>Triage reports</h2>
  <table>
    <thead><tr><th>Path</th><th>Buckets</th><th>Crashes</th><th>Timeouts</th></tr></thead>
    <tbody>{triage_rows}</tbody>
  </table>
</body>
</html>
'''
    (out_dir / 'index.html').write_text(html_doc, encoding='utf-8')


def main() -> int:
    ap = argparse.ArgumentParser(description='Render CI bundle summary as JSON/HTML')
    ap.add_argument('--out-dir', required=True)
    ap.add_argument('--project', required=True)
    ap.add_argument('--target', required=True)
    args = ap.parse_args()

    out_dir = Path(args.out_dir)
    manifest = parse_manifest_txt(out_dir / 'manifest.txt')
    files = collect_files(out_dir)
    triage_reports = []
    for p in out_dir.rglob('report.json'):
        if p.name != 'report.json':
            continue
        data = load_json_if_exists(p)
        if not isinstance(data, dict):
            continue
        triage_reports.append({
            'path': rel(p, out_dir),
            'mode': data.get('mode', ''),
            'buckets_total': data.get('buckets_total', 0),
            'cases_crash': data.get('cases_crash', 0),
            'cases_timeout': data.get('cases_timeout', 0),
        })

    source_corpus_path = Path(manifest['source_corpus']) if manifest.get('source_corpus') else None
    minimized_corpus_path = Path(manifest['minimized_corpus']) if manifest.get('minimized_corpus') else None

    index_data = {
        'project': args.project,
        'target': args.target,
        'generated_utc': manifest.get('generated_utc', ''),
        'harness': manifest.get('harness', ''),
        'source_corpus': manifest.get('source_corpus', ''),
        'source_corpus_files': int(manifest.get('source_corpus_files', count_non_hidden(source_corpus_path))),
        'minimized_corpus': manifest.get('minimized_corpus', ''),
        'minimized_corpus_files': int(manifest.get('minimized_corpus_files', count_non_hidden(minimized_corpus_path))),
        'files_total': len(files),
        'files': files,
        'triage_reports': triage_reports,
    }

    # First pass: emit summary artifacts.
    (out_dir / 'summary.md').write_text(
        f"## {args.project}:{args.target} artifact bundle\n\n"
        f"- Total files: {index_data['files_total']}\n"
        f"- Source corpus files: {index_data['source_corpus_files']}\n"
        f"- Minimized corpus files: {index_data['minimized_corpus_files']}\n"
        f"- Triage reports: {len(triage_reports)}\n",
        encoding='utf-8'
    )
    write_html(out_dir, index_data)
    (out_dir / 'manifest.json').write_text(json.dumps(index_data, indent=2, sort_keys=True), encoding='utf-8')
    (out_dir / 'index.json').write_text(json.dumps(index_data, indent=2, sort_keys=True), encoding='utf-8')

    # Second pass: refresh file inventory so the generated summaries are also indexed.
    refreshed_files = collect_files(out_dir)
    index_data['files'] = refreshed_files
    index_data['files_total'] = len(refreshed_files)
    (out_dir / 'summary.md').write_text(
        f"## {args.project}:{args.target} artifact bundle\n\n"
        f"- Total files: {index_data['files_total']}\n"
        f"- Source corpus files: {index_data['source_corpus_files']}\n"
        f"- Minimized corpus files: {index_data['minimized_corpus_files']}\n"
        f"- Triage reports: {len(triage_reports)}\n",
        encoding='utf-8'
    )
    write_html(out_dir, index_data)
    (out_dir / 'manifest.json').write_text(json.dumps(index_data, indent=2, sort_keys=True), encoding='utf-8')
    (out_dir / 'index.json').write_text(json.dumps(index_data, indent=2, sort_keys=True), encoding='utf-8')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
