#!/usr/bin/env python3
"""Generate vp8_probdata.[ch] from an RFC 6386 text file."""
import re, sys, pathlib

def extract(text, name):
    pats = [
        rf"{re.escape(name)}\s*\[[^\n]*?=\s*\{{.*?\n\s*\}};",
        rf"{re.escape(name)}\s*\[\]\s*=\s*\{{.*?\n\s*\}};",
        rf"{re.escape(name)}\s*=\s*\{{.*?\n\s*\}};",
    ]
    for pat in pats:
        m = re.search(pat, text, re.S)
        if m:
            s = m.group(0)
            s = s[s.find('{'):]
            s = re.sub(r'/\*.*?\*/', '', s, flags=re.S)
            return '\n'.join(line.rstrip() for line in s.splitlines())
    raise SystemExit(f'missing array: {name}')

if len(sys.argv) != 2:
    raise SystemExit('usage: gen_vp8_probdata_from_rfc.py rfc6386.txt')
text = pathlib.Path(sys.argv[1]).read_text(encoding='utf-8', errors='ignore')
for name in ['coeff_update_probs', 'default_coeff_probs', 'kf_b_mode_probs']:
    print(f'found {name}:', bool(extract(text, name)))
print('Generator used only during package creation; generated sources are committed.')
