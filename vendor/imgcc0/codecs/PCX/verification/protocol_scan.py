from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
files = list(ROOT.glob('*.c')) + list(ROOT.glob('*.h')) + list((ROOT / 'include' / 'pcx').glob('*.h'))
patterns = {
    'dynamic-memory-token': re.compile(r'\b(?:malloc|calloc|realloc|free|heap)\b', re.I),
    'floating-token': re.compile(r'\b(?:float|double)\b'),
    'wide-integer-token': re.compile(r'\b(?:long\s+long|int64_t|uint64_t|intptr_t|uintptr_t|size_t)\b|\bULL\b|[0-9A-Fa-fx]+UL\b'),
    'explicit-long-type': re.compile(r'\b(?:unsigned\s+long|signed\s+long|long)\b'),
}
fail = []
for path in files:
    text = path.read_text(errors='replace')
    for name, rx in patterns.items():
        for m in rx.finditer(text):
            line = text.count('\n', 0, m.start()) + 1
            fail.append((path.relative_to(ROOT).as_posix(), line, name, m.group(0)))
if fail:
    for item in fail:
        print('%s:%d: %s: %r' % item)
    sys.exit(1)
print('PASS: %d production source/header files satisfy the PCX89 token protocol.' % len(files))
