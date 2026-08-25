from pathlib import Path
import hashlib
import subprocess
import tempfile
import sys

ROOT = Path(__file__).resolve().parents[1]
DEMO = ROOT / "build" / "demo_cli"
ASSETS = ROOT / "demo_assets" / "input"
EXPECTED = {
    "sample.jpg": ["45f1dfd94b7a72e18e09810e8160f0d7b0c48eb8d0a288fa0274825a05566724"],
    "sample.tga": ["1fb8b3c2c27b30f4ddd64fe3b8e944c73a86aaecde68b89f517bfc556b04cde9"],
    "sample.qoi": ["1fb8b3c2c27b30f4ddd64fe3b8e944c73a86aaecde68b89f517bfc556b04cde9"],
    "sample.webp": ["1fb8b3c2c27b30f4ddd64fe3b8e944c73a86aaecde68b89f517bfc556b04cde9"],
    "sample.tiff": ["1fb8b3c2c27b30f4ddd64fe3b8e944c73a86aaecde68b89f517bfc556b04cde9"],
    "sample.gif": [
        "5e8a80b3228de33076ea918bed5daea5ea137989ed3c06d41a1fca04ed6ae5b6",
        "65b08d82c1f26152623264ac1af972e5f586d84ba13914d37ef7d4f41034904b",
        "30ebfd5f94c9a81a4859e85f7040630c30dfd9b204c3f288d1ee181edfdf46ed",
        "7426533808e254e1bd2ec78c4a98dbcc99167e101a0c0d8041c4d2be9c26c8b9",
    ],
    "sample_anim.webp": [
        "0277539f828c0f990760a16334aa36eeee04cd6f74a3b75e30a5af045946b2bc",
        "349701fd2a2db942788887e8a92a09e5c1f5dd08e4687462f822cdf9c718a7c8",
        "cdf26cdb6c19a0439c730b970221154e815e063619a743041646965418ff0ac2",
        "d036a459a0b76a5bf0f1709f121d3596a63f62b6b7569fafc4c6bab65b5cd7e4",
    ],
}

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

if not DEMO.exists():
    print("build/demo_cli is missing; run make first")
    sys.exit(2)

with tempfile.TemporaryDirectory(prefix="imgcc0-reg-") as td:
    td = Path(td)
    failed = False
    count = 0
    for name, hashes in EXPECTED.items():
        prefix = td / name.replace(".", "_")
        subprocess.check_call([str(DEMO), str(ASSETS / name), "--dump-prefix", str(prefix)], stdout=subprocess.DEVNULL)
        for i, expected in enumerate(hashes):
            out = Path("%s_%03d.pam" % (prefix, i))
            got = digest(out)
            count += 1
            if got != expected:
                print("FAIL %s frame %d\n  expected %s\n  got      %s" % (name, i, expected, got))
                failed = True
            else:
                print("PASS %s frame %d" % (name, i))
    if failed:
        sys.exit(1)
    print("strict regression: PASS (%d byte-identical frame outputs)" % count)
