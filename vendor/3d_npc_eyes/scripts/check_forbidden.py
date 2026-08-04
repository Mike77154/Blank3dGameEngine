#!/usr/bin/env python3
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
SCAN_DIRS = [ROOT / "include", ROOT / "src", ROOT / "examples", ROOT / "tests"]
FORBIDDEN = [
    r"\bmalloc\s*\(",
    r"\bcalloc\s*\(",
    r"\brealloc\s*\(",
    r"\bfree\s*\(",
    r"\bfloat\b",
    r"\bdouble\b",
    r"[0-9]+\.[0-9]+",
]

errors = []
for folder in SCAN_DIRS:
    for path in folder.rglob("*"):
        if path.suffix.lower() not in (".c", ".h"):
            continue
        text = path.read_text(errors="replace")
        for pattern in FORBIDDEN:
            if re.search(pattern, text):
                errors.append((path.relative_to(ROOT), pattern))

if errors:
    print("Forbidden constructs found:")
    for path, pattern in errors:
        print("  %s -> %s" % (path, pattern))
    sys.exit(1)

print("No forbidden constructs found in public/source/example/test C files.")
