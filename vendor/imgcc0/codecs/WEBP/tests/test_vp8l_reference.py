#!/usr/bin/env python3
import os
import subprocess
import sys
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parent.parent
OUT = Path(os.environ.get("GWP_TEST_TMP", "/tmp/giffywebp-tests"))
OUT.mkdir(parents=True, exist_ok=True)


def read_pam(path: Path):
    data = path.read_bytes()
    header_end = data.index(b"ENDHDR\n") + 7
    header = data[:header_end].decode("ascii")
    meta = {}
    for line in header.splitlines()[1:]:
        if line == "ENDHDR":
            break
        k, v = line.split(" ", 1)
        meta[k] = v
    return int(meta["WIDTH"]), int(meta["HEIGHT"]), data[header_end:]


def make_case(name, img: Image.Image):
    png = OUT / f"{name}.png"
    webp = OUT / f"{name}.webp"
    ref = OUT / f"{name}_ref.png"
    pam = OUT / f"{name}.pam"
    img.save(png)
    subprocess.run([
        "ffmpeg", "-y", "-loglevel", "error", "-i", str(png),
        "-c:v", "libwebp", "-lossless", "1", str(webp)
    ], check=True)
    subprocess.run([
        str(ROOT / "examples" / "gwpdecode"), str(webp), str(pam)
    ], check=True)
    subprocess.run([
        "ffmpeg", "-y", "-loglevel", "error", "-i", str(webp), str(ref)
    ], check=True)

    w, h, ours = read_pam(pam)
    ref_img = Image.open(ref).convert("RGBA")
    assert ref_img.size == (w, h), (name, ref_img.size, (w, h))
    if ref_img.tobytes() != ours:
        raise AssertionError(f"reference mismatch: {name}")


def build_cases():
    cases = []

    img = Image.new("RGBA", (16, 16), (255, 0, 0, 255))
    cases.append(("solid", img))

    img = Image.new("RGBA", (32, 24))
    p = img.load()
    for y in range(img.height):
        for x in range(img.width):
            p[x, y] = (x * 8 % 256, y * 10 % 256, (x * 3 + y * 5) % 256, 255)
    cases.append(("gradient", img))

    img = Image.new("RGBA", (20, 20))
    p = img.load()
    for y in range(img.height):
        for x in range(img.width):
            p[x, y] = ((x * 11) % 256, (y * 9) % 256, ((x + y) * 7) % 256,
                       (x * 13 + y * 17) % 256)
    cases.append(("alpha", img))

    img = Image.new("RGBA", (40, 10))
    p = img.load()
    colors = [(255, 0, 0, 255), (0, 255, 0, 255), (0, 0, 255, 255), (255, 255, 0, 255)]
    for y in range(img.height):
        for x in range(img.width):
            p[x, y] = colors[(x // 2 + y) % len(colors)]
    cases.append(("palette", img))

    img = Image.new("RGBA", (17, 13))
    p = img.load()
    seed = 0xC0FFEE
    for y in range(img.height):
        for x in range(img.width):
            seed = (1103515245 * seed + 12345) & 0x7FFFFFFF
            r = seed & 255
            seed = (1103515245 * seed + 12345) & 0x7FFFFFFF
            g = seed & 255
            seed = (1103515245 * seed + 12345) & 0x7FFFFFFF
            b = seed & 255
            seed = (1103515245 * seed + 12345) & 0x7FFFFFFF
            a = seed & 255
            p[x, y] = (r, g, b, a)
    cases.append(("random", img))

    img = Image.new("RGBA", (96, 64))
    p = img.load()
    for y in range(img.height):
        for x in range(img.width):
            a = 255 if ((x ^ y) & 7) else 0
            p[x, y] = ((x * 5 + y * 7) & 255, (x * 11) & 255, (y * 13) & 255, a)
    cases.append(("metaish", img))

    return cases


def main():
    for name, img in build_cases():
        make_case(name, img)
        print(f"ok {name}")
    print("VP8L reference corpus passed")


if __name__ == "__main__":
    sys.exit(main())
