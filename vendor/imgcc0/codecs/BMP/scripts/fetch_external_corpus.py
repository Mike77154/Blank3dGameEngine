#!/usr/bin/env python3
import argparse
import json
import pathlib
import sys
import urllib.request


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--manifest", default="tests/external_corpus/bmp_manifest.json")
    parser.add_argument("--output-dir", default="tests/external_corpus/downloaded")
    args = parser.parse_args()

    manifest_path = pathlib.Path(args.manifest)
    data = json.loads(manifest_path.read_text())
    output_dir = pathlib.Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    presets = data.get("source_presets", {})
    fetched = 0
    for case in data.get("cases", []):
        preset = presets[case["source"]]
        url = preset["base_url"] + case["relative_path"]
        dst = output_dir / case["source"] / case["relative_path"]
        dst.parent.mkdir(parents=True, exist_ok=True)
        try:
            urllib.request.urlretrieve(url, dst)
            fetched += 1
            print(f"fetched {case['id']} -> {dst}")
        except Exception as exc:  # pragma: no cover
            print(f"warning: failed to fetch {url}: {exc}", file=sys.stderr)
    print(f"fetched {fetched} files")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
