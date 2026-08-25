#!/usr/bin/env python3
import json
import shutil

TOOLS = ['cwebp', 'dwebp', 'img2webp', 'gif2webp', 'webpmux', 'webpinfo']

result = {}
for name in TOOLS:
    result[name] = shutil.which(name)
print(json.dumps(result, indent=2, sort_keys=True))
