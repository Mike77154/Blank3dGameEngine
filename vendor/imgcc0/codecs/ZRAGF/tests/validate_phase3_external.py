#!/usr/bin/env python3
import subprocess
import sys
import zlib

def xorshift32_stream(n: int) -> bytes:
    state = 0x12345678
    out = bytearray()
    for _ in range(n):
        state ^= (state << 13) & 0xFFFFFFFF
        state ^= (state >> 17) & 0xFFFFFFFF
        state ^= (state << 5) & 0xFFFFFFFF
        out.append(state & 0xFF)
    return bytes(out)

def payload(case: str) -> bytes:
    if case == "dynamic":
        return (b"a" * 1000) + (b"b" * 1000) + (b"c" * 1000)
    if case == "fixed":
        return bytes([i & 0xFF for i in range(1024)])
    if case == "stored":
        return xorshift32_stream(8192)
    raise ValueError(case)

def expect_btype(case: str) -> int:
    return {"dynamic": 2, "fixed": 1, "stored": 0}[case]

def get_btype(comp: bytes) -> int:
    return (comp[0] >> 1) & 0x03

def validate(helper: str, case: str) -> None:
    proc = subprocess.run([helper, case], stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True)
    comp = proc.stdout
    got_btype = get_btype(comp)
    want_btype = expect_btype(case)
    if got_btype != want_btype:
        raise SystemExit(f"{case}: expected BTYPE {want_btype}, got {got_btype}")
    out = zlib.decompress(comp, -15)
    if out != payload(case):
        raise SystemExit(f"{case}: external zlib payload mismatch")
    print(f"{case}: BTYPE={got_btype} external-zlib=ok in={len(out)} out={len(comp)}")

def main() -> None:
    if len(sys.argv) != 2:
        raise SystemExit("usage: validate_phase3_external.py <phase3_emit_case_binary>")
    helper = sys.argv[1]
    for case in ("dynamic", "fixed", "stored"):
        validate(helper, case)

if __name__ == "__main__":
    main()
