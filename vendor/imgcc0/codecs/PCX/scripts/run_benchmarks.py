#!/usr/bin/env python3
import argparse
import json
import pathlib
import re
import subprocess


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--bench", default="tools/pcx_bench")
    parser.add_argument("--loops", type=int, default=256)
    parser.add_argument("--json-out", default="tests/perf_reports/benchmark.json")
    parser.add_argument("--md-out", default="tests/perf_reports/benchmark.md")
    args = parser.parse_args()
    root = pathlib.Path(__file__).resolve().parent.parent
    proc = subprocess.run([str((root / args.bench).resolve()), str(args.loops)], check=True, capture_output=True, text=True)
    text = proc.stdout
    metrics = {}
    for key in ["inspect", "decode", "encode"]:
        m = re.search(rf"{key}:\s+([0-9.]+) s total, ([0-9.]+) us/op", text)
        if m:
            metrics[key] = {"seconds_total": float(m.group(1)), "us_per_op": float(m.group(2))}
    m = re.search(r"encodedBytes=(\d+)", text)
    if m:
        metrics["encoded_bytes"] = int(m.group(1))
    payload = {"loops": args.loops, "metrics": metrics, "raw_output": text}

    json_path = root / args.json_out
    md_path = root / args.md_out
    json_path.parent.mkdir(parents=True, exist_ok=True)
    md_path.parent.mkdir(parents=True, exist_ok=True)
    json_path.write_text(json.dumps(payload, indent=2) + "\n")
    lines = ["# PCX benchmark report", "", f"Loops: {args.loops}", "", "| Operation | us/op |", "|---|---:|"]
    for key in ["inspect", "decode", "encode"]:
        if key in metrics:
            lines.append(f"| {key} | {metrics[key]['us_per_op']:.3f} |")
    md_path.write_text("\n".join(lines) + "\n")
    print(md_path)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
