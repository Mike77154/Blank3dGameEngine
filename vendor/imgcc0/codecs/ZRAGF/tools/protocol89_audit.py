#!/usr/bin/env python3
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "PROTOCOL89_PRESERVATION_MANIFEST.txt"

RULES = [
    ("runtime heap take", re.compile(r"\bmalloc\b")),
    ("runtime heap zero-take", re.compile(r"\bcalloc\b")),
    ("runtime resize", re.compile(r"\brealloc\b")),
    ("runtime release", re.compile(r"\bfree\b")),
    ("binary fp32", re.compile(r"\bfloat\b")),
    ("binary fp64", re.compile(r"\bdouble\b")),
    ("wide integer", re.compile(r"\blong\s+long\b")),
    ("fixed-width 64", re.compile(r"\b(?:u?int64_t)\b")),
    ("pointer-width integer", re.compile(r"\b(?:u?intptr_t)\b")),
    ("C99 integer header", re.compile(r"<stdint\.h>")),
    ("C99 bounded formatter", re.compile(r"\bsnprintf\b")),
]


def main():
    errors = []
    code_files = sorted(list(ROOT.rglob("*.c")) + list(ROOT.rglob("*.h")))
    for path in code_files:
        text = path.read_text(errors="replace")
        for label, rx in RULES:
            for m in rx.finditer(text):
                line = text.count("\n", 0, m.start()) + 1
                errors.append("%s:%d: %s: %s" % (path.relative_to(ROOT), line, label, m.group(0)))
    if MANIFEST.exists():
        for raw in MANIFEST.read_text().splitlines():
            raw = raw.strip()
            if raw and not (ROOT / raw).is_file():
                errors.append("missing preserved file: %s" % raw)
    else:
        errors.append("missing preservation manifest")

    required = [
        ROOT / "tests",
        ROOT / "fuzz",
        ROOT / "bench",
        ROOT / "bench" / "corpora",
        ROOT / "tools",
        ROOT / "docs",
        ROOT / "examples",
    ]
    for path in required:
        if not path.exists():
            errors.append("missing infrastructure path: %s" % path.relative_to(ROOT))

    if errors:
        for error in errors:
            print(error, file=sys.stderr)
        print("protocol89 audit failed: %d issue(s)" % len(errors), file=sys.stderr)
        return 1

    print("protocol89 audit ok: %d C/H files; %d preserved original files" %
          (len(code_files), len(MANIFEST.read_text().splitlines())))
    return 0

if __name__ == "__main__":
    sys.exit(main())
