#!/usr/bin/env python3
import hashlib
import pathlib
import subprocess
import sys

EXPECTED_SHA256 = "13e5b87531e593d66dfb00db14b85ce0dc25a46990c7c91256b9dc5b5eaa0107"

def main():
    if len(sys.argv) != 3:
        print("usage: validate_protocol89_byte_exact.py <emitter> <golden>", file=sys.stderr)
        return 2
    emitter = pathlib.Path(sys.argv[1])
    golden_path = pathlib.Path(sys.argv[2])
    golden = golden_path.read_bytes()
    got = subprocess.check_output([str(emitter)])
    golden_sha = hashlib.sha256(golden).hexdigest()
    got_sha = hashlib.sha256(got).hexdigest()
    if golden_sha != EXPECTED_SHA256:
        print("golden checksum drift: %s" % golden_sha, file=sys.stderr)
        return 3
    if got != golden:
        limit = min(len(got), len(golden))
        offset = None
        for i in range(limit):
            if got[i] != golden[i]:
                offset = i
                break
        if offset is None:
            offset = limit
        print("byte mismatch at offset %d; expected_bytes=%d got_bytes=%d expected_sha=%s got_sha=%s" %
              (offset, len(golden), len(got), golden_sha, got_sha), file=sys.stderr)
        return 1
    print("protocol89 byte-exact ok: bytes=%d sha256=%s" % (len(got), got_sha))
    return 0

if __name__ == "__main__":
    sys.exit(main())
