#!/usr/bin/env python3
import argparse
import json
import pathlib
import re
import subprocess


SRC_FILES = [
    "bmp/bmp_chunks.c",
    "bmp/bmp_metadata.c",
    "bmp/bmp_parser.c",
    "bmp/bmp_report.c",
    "bmp/bmp_decoder.c",
    "bmp/bmp_encoder.c",
    "bmp/bmp_filters.c",
    "bmp/bmp_render.c",
]


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--json-out", default="tests/coverage_reports/coverage.json")
    parser.add_argument("--md-out", default="tests/coverage_reports/coverage.md")
    args = parser.parse_args()
    root = pathlib.Path(__file__).resolve().parent.parent

    subprocess.run(["make", "clean"], cwd=root, check=True)
    subprocess.run([
        "make", "test",
        "CFLAGS=-std=c89 -pedantic-errors -O0 --coverage -Wall -Wextra -Wpedantic",
        "LDFLAGS=--coverage",
    ], cwd=root, check=True)

    metrics = []
    for src in SRC_FILES:
        proc = subprocess.run(["gcov", "-o", "bmp", src], cwd=root, check=True, capture_output=True, text=True)
        m = re.search(r"Lines executed:([0-9.]+)% of (\d+)", proc.stdout)
        if m:
            metrics.append({"file": src, "line_coverage_percent": float(m.group(1)), "lines": int(m.group(2))})

    payload = {"files": metrics}
    json_path = root / args.json_out
    md_path = root / args.md_out
    json_path.parent.mkdir(parents=True, exist_ok=True)
    md_path.parent.mkdir(parents=True, exist_ok=True)
    json_path.write_text(json.dumps(payload, indent=2) + "\n")
    lines = ["# BMP coverage report", "", "| File | Line coverage % |", "|---|---:|"]
    for item in metrics:
        lines.append(f"| {item['file']} | {item['line_coverage_percent']:.2f} |")
    md_path.write_text("\n".join(lines) + "\n")
    print(md_path)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
