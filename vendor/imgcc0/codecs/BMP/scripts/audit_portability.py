#!/usr/bin/env python3
import argparse
import json
from pathlib import Path

REQUIRED_CONFIGURE = [
    "linux-gcc-release",
    "linux-clang-release",
    "macos-clang-release",
    "windows-msvc-release",
]


def load_json(path: Path):
    return json.loads(path.read_text(encoding="utf-8"))


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--repo-root", required=True)
    ap.add_argument("--json-out", required=True)
    ap.add_argument("--md-out", required=True)
    args = ap.parse_args()

    root = Path(args.repo_root)
    preset_path = root / "CMakePresets.json"
    ci_path = root / ".github" / "workflows" / "ci.yml"
    cmake_path = root / "CMakeLists.txt"
    report = {"ok": True, "checks": []}

    def check(name: str, ok: bool, details: str) -> None:
        report["checks"].append({"name": name, "ok": ok, "details": details})
        if not ok:
            report["ok"] = False

    if preset_path.exists():
        presets = load_json(preset_path)
        configure_names = [p.get("name") for p in presets.get("configurePresets", [])]
        build_names = [p.get("name") for p in presets.get("buildPresets", [])]
        test_names = [p.get("name") for p in presets.get("testPresets", [])]
        for name in REQUIRED_CONFIGURE:
            check(f"configure preset {name}", name in configure_names, f"available configure presets: {configure_names}")
            check(f"build preset {name}", name in build_names, f"available build presets: {build_names}")
            check(f"test preset {name}", name in test_names, f"available test presets: {test_names}")
    else:
        check("CMakePresets.json exists", False, "missing preset file")

    ci_text = ci_path.read_text(encoding="utf-8") if ci_path.exists() else ""
    check("ci workflow portability job", "portability-matrix:" in ci_text, "expected portability-matrix job in .github/workflows/ci.yml")
    for token in ["ubuntu-latest", "macos-latest", "windows-latest"]:
        check(f"ci matrix contains {token}", token in ci_text, "runner token search")
    for token in REQUIRED_CONFIGURE:
        check(f"ci references {token}", token in ci_text, "preset token search")

    cmake_text = cmake_path.read_text(encoding="utf-8") if cmake_path.exists() else ""
    check("CMake uses CTest", "include(CTest)" in cmake_text, "expected include(CTest)")
    check("CMake enables Windows export-all-symbols", "WINDOWS_EXPORT_ALL_SYMBOLS" in cmake_text, "expected WINDOWS_EXPORT_ALL_SYMBOLS property or variable")
    check("CMake sets MSVC runtime library", "MSVC_RUNTIME_LIBRARY" in cmake_text, "expected MSVC runtime property")
    check("CMake sets MSVC /W4 /WX", "/W4" in cmake_text and "/WX" in cmake_text, "expected /W4 and /WX in compile options")

    json_out = Path(args.json_out)
    md_out = Path(args.md_out)
    json_out.parent.mkdir(parents=True, exist_ok=True)
    md_out.parent.mkdir(parents=True, exist_ok=True)
    json_out.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    lines = [
        "# Portability audit",
        "",
        f"Overall: {'PASS' if report['ok'] else 'FAIL'}",
        "",
        "| Check | Result | Details |",
        "|---|---|---|",
    ]
    for item in report["checks"]:
        lines.append(f"| {item['name']} | {'PASS' if item['ok'] else 'FAIL'} | {item['details']} |")
    md_out.write_text("\n".join(lines) + "\n", encoding="utf-8")
    return 0 if report["ok"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
