#!/usr/bin/env python3
import sys
from pathlib import Path


def read_data(argv):
    if len(argv) > 1:
        return Path(argv[1]).read_bytes()
    return sys.stdin.buffer.read()


def emit(summary, frames):
    sys.stderr.write(f"ERROR: AddressSanitizer: {summary}\n")
    for idx, frame in enumerate(frames):
        sys.stderr.write(f"    #{idx} 0x{idx+1:x} in {frame} /tmp/mock.c:{10 + idx}\n")
    sys.stderr.write(f"SUMMARY: AddressSanitizer: {summary} in {frames[0]}\n")


def emit_ubsan(summary, frames):
    sys.stderr.write(f"runtime error: {summary}\n")
    for idx, frame in enumerate(frames):
        sys.stderr.write(f"    #{idx} 0x{idx+1:x} in {frame} /tmp/mock.c:{20 + idx}\n")


def main(argv):
    data = read_data(argv)
    if data.startswith(b'OVER'):
        emit('heap-buffer-overflow on address 0x1234', ['parse_header', 'fuzz_entry'])
        return 1
    if data.startswith(b'NULL'):
        emit('SEGV on unknown address 0x0', ['decode_row', 'fuzz_entry'])
        return 1
    if data.startswith(b'UBSN'):
        emit_ubsan("signed integer overflow: 1 + 2147483647 cannot be represented in type 'int'", ['parse_dims', 'fuzz_entry'])
        return 1
    return 0


if __name__ == '__main__':
    raise SystemExit(main(sys.argv))
