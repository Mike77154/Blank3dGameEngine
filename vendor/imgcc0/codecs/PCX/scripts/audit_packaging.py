#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import py_compile
import tarfile
import zipfile
from pathlib import Path


def read_json(path: Path):
    return json.loads(path.read_text(encoding="utf-8"))


def archive_names(path: Path):
    if path.suffix == ".zip":
        with zipfile.ZipFile(path) as zf:
            return zf.namelist()
    with tarfile.open(path, "r:*") as tf:
        return tf.getnames()


def contains_suffix(entries, suffix):
    return any(entry.endswith(suffix) for entry in entries)


def render_markdown(result: dict) -> str:
    lines = ["# Packaging audit", ""]
    lines.append(f"Errors: {len(result['errors'])}")
    lines.append(f"Warnings: {len(result['warnings'])}")
    lines.append("")
    if result["artifacts"]:
        lines.append("## Archives checked")
        for item in result["artifacts"]:
            lines.append(f"- `{item}`")
        lines.append("")
    if result["errors"]:
        lines.append("## Errors")
        for item in result["errors"]:
            lines.append(f"- `{item['where']}`: {item['message']}")
        lines.append("")
    if result["warnings"]:
        lines.append("## Warnings")
        for item in result["warnings"]:
            lines.append(f"- `{item['where']}`: {item['message']}")
        lines.append("")
    return "\n".join(lines) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser(description="Audit packaging metadata and generated package archives.")
    parser.add_argument("--repo-root", default=".")
    parser.add_argument("--project", required=True)
    parser.add_argument("--version", required=True)
    parser.add_argument("--build-dir")
    parser.add_argument("--json-out")
    parser.add_argument("--md-out")
    args = parser.parse_args()

    repo_root = Path(args.repo_root).resolve()
    errors = []
    warnings = []
    artifacts = []

    version_text = (repo_root / "VERSION").read_text(encoding="utf-8").strip()
    if version_text != args.version:
        errors.append({"where": "VERSION", "message": f"version file says {version_text}, expected {args.version}"})

    conan_recipe = repo_root / "conanfile.py"
    test_recipe = repo_root / "test_package" / "conanfile.py"
    for candidate in [conan_recipe, test_recipe]:
        if not candidate.exists():
            errors.append({"where": str(candidate.relative_to(repo_root)), "message": "missing Conan recipe"})
        else:
            try:
                py_compile.compile(str(candidate), doraise=True)
            except py_compile.PyCompileError as exc:
                errors.append({"where": str(candidate.relative_to(repo_root)), "message": f"python compile failed: {exc.msg}"})

    port_json = repo_root / "packaging" / "vcpkg" / "ports" / args.project / "vcpkg.json"
    portfile = repo_root / "packaging" / "vcpkg" / "ports" / args.project / "portfile.cmake"
    usage_file = repo_root / "packaging" / "vcpkg" / "ports" / args.project / "usage"
    if not port_json.exists():
        errors.append({"where": str(port_json.relative_to(repo_root)), "message": "missing vcpkg manifest"})
    else:
        manifest = read_json(port_json)
        if manifest.get("name") != args.project:
            errors.append({"where": str(port_json.relative_to(repo_root)), "message": f"name mismatch: {manifest.get('name')}"})
        if manifest.get("version-string") != args.version:
            errors.append({"where": str(port_json.relative_to(repo_root)), "message": f"version mismatch: {manifest.get('version-string')}"})
    for candidate in [portfile, usage_file, repo_root / "examples" / "pkgconfig_consumer" / "main.c"]:
        if not candidate.exists():
            errors.append({"where": str(candidate.relative_to(repo_root)), "message": "required packaging file missing"})

    cmake_lists = (repo_root / "CMakeLists.txt").read_text(encoding="utf-8")
    if "include(CPack)" not in cmake_lists:
        errors.append({"where": "CMakeLists.txt", "message": "CPack integration missing"})

    if args.build_dir:
        build_dir = Path(args.build_dir).resolve()
        if not build_dir.exists():
            errors.append({"where": str(build_dir), "message": "build directory does not exist"})
        else:
            archive_candidates = sorted([p for p in build_dir.iterdir() if p.name.startswith(f"{args.project}-{args.version}") and (p.suffix == ".zip" or p.name.endswith(".tar.gz"))])
            if not archive_candidates:
                errors.append({"where": str(build_dir.relative_to(repo_root)), "message": "no CPack archives found"})
            source_seen = False
            binary_seen = False
            for archive in archive_candidates:
                entries = archive_names(archive)
                artifacts.append(str(archive.relative_to(repo_root)))
                if "-source" in archive.name:
                    source_seen = True
                    if not contains_suffix(entries, "CMakeLists.txt"):
                        errors.append({"where": str(archive.relative_to(repo_root)), "message": "source package missing CMakeLists.txt"})
                    if not contains_suffix(entries, "conanfile.py"):
                        errors.append({"where": str(archive.relative_to(repo_root)), "message": "source package missing conanfile.py"})
                    if not contains_suffix(entries, "packaging/vcpkg/README.md"):
                        errors.append({"where": str(archive.relative_to(repo_root)), "message": "source package missing vcpkg docs"})
                else:
                    binary_seen = True
                    expected_suffixes = [
                        f"lib/lib{args.project}",
                        f"include/{args.project}/",
                        f"lib/pkgconfig/{args.project}.pc",
                        f"lib/cmake/{args.project}/",
                    ]
                    for suffix in expected_suffixes:
                        if not any(suffix in entry for entry in entries):
                            errors.append({"where": str(archive.relative_to(repo_root)), "message": f"binary package missing payload similar to {suffix}"})
            if not source_seen:
                errors.append({"where": str(build_dir.relative_to(repo_root)), "message": "missing source package archive"})
            if not binary_seen:
                errors.append({"where": str(build_dir.relative_to(repo_root)), "message": "missing binary package archive"})

    result = {"errors": errors, "warnings": warnings, "artifacts": artifacts}
    if args.json_out:
        out = Path(args.json_out)
        out.parent.mkdir(parents=True, exist_ok=True)
        out.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    if args.md_out:
        out = Path(args.md_out)
        out.parent.mkdir(parents=True, exist_ok=True)
        out.write_text(render_markdown(result), encoding="utf-8")
    if errors:
        raise SystemExit("packaging audit failed")
    print("audit_packaging: OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
