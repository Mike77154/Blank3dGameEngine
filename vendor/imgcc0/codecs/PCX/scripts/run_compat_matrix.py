#!/usr/bin/env python3
import argparse
import json
import pathlib
import subprocess
from typing import Any, Dict, List


def load_cases(root: pathlib.Path) -> List[Dict[str, Any]]:
    cases: List[Dict[str, Any]] = []
    generated_manifest = root / "tests" / "generated_corpus" / "manifest.json"
    if generated_manifest.exists():
        data = json.loads(generated_manifest.read_text())
        for case in data.get("cases", []):
            case = dict(case)
            case["resolved_path"] = str((root / "tests" / "generated_corpus" / case["path"]).resolve())
            case["origin"] = "generated"
            cases.append(case)
    external_manifest = root / "tests" / "external_corpus" / "pcx_manifest.json"
    downloaded_root = root / "tests" / "external_corpus" / "downloaded"
    if external_manifest.exists():
        data = json.loads(external_manifest.read_text())
        for case in data.get("cases", []):
            case = dict(case)
            case["resolved_path"] = str((downloaded_root / case["source"] / case["relative_path"]).resolve())
            case["origin"] = "external"
            cases.append(case)
    return cases


def eval_case(cli: pathlib.Path, case: Dict[str, Any]) -> Dict[str, Any]:
    result = dict(case)
    path = pathlib.Path(case["resolved_path"])
    if not path.exists():
        result["status"] = "missing"
        return result
    proc = subprocess.run([str(cli), "--json", str(path)], check=False, capture_output=True, text=True)
    result["status"] = "ran"
    result["cli_returncode"] = proc.returncode
    if proc.stdout.strip():
        result["observed"] = json.loads(proc.stdout)
    else:
        result["observed"] = {"inspect_ok": False, "rgb_ok": False, "indexed_ok": False, "strict_ok": False}
    expected = case.get("expected", {})
    ok = True
    for key, value in expected.items():
        if result["observed"].get(key) != value:
            ok = False
    result["matches_expectation"] = ok
    return result


def render_md(results: List[Dict[str, Any]]) -> str:
    lines = ["# PCX compatibility matrix", "", "| Case | Origin | Inspect | RGB | Indexed | Strict | Expectation |", "|---|---|---:|---:|---:|---:|---:|"]
    for item in results:
        if item.get("status") == "missing":
            lines.append(f"| {item['id']} | {item['origin']} | n/a | n/a | n/a | n/a | missing |")
            continue
        obs = item["observed"]
        lines.append(
            f"| {item['id']} | {item['origin']} | {obs.get('inspect_ok')} | {obs.get('rgb_ok')} | {obs.get('indexed_ok')} | {obs.get('strict_ok')} | {item.get('matches_expectation')} |"
        )
    return "\n".join(lines) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--cli", default="tools/pcx_compat_cli")
    parser.add_argument("--json-out", default="tests/compat_reports/compat_matrix.json")
    parser.add_argument("--md-out", default="tests/compat_reports/compat_matrix.md")
    args = parser.parse_args()
    root = pathlib.Path(__file__).resolve().parent.parent
    cli = (root / args.cli).resolve()
    results = [eval_case(cli, case) for case in load_cases(root)]
    passed = sum(1 for x in results if x.get("matches_expectation") is True)
    executed = sum(1 for x in results if x.get("status") == "ran")
    summary = {"executed": executed, "passed": passed, "results": results}

    json_path = root / args.json_out
    md_path = root / args.md_out
    json_path.parent.mkdir(parents=True, exist_ok=True)
    md_path.parent.mkdir(parents=True, exist_ok=True)
    json_path.write_text(json.dumps(summary, indent=2) + "\n")
    md_path.write_text(render_md(results))
    print(f"compat matrix: {passed}/{executed} matched expectations")
    print(md_path)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
