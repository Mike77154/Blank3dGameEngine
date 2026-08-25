from __future__ import annotations

from pathlib import Path
from PIL import Image, ImageDraw
import json

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "demo_assets" / "input"
OUT.mkdir(parents=True, exist_ok=True)
Image.init()


def make_base_rgba(w: int = 96, h: int = 96) -> Image.Image:
    im = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    px = im.load()
    for y in range(h):
        for x in range(w):
            checker = 30 if ((x // 12) + (y // 12)) % 2 else 220
            r = (x * 255) // max(1, w - 1)
            g = (y * 255) // max(1, h - 1)
            b = checker
            a = 180
            if 18 < x < 78 and 18 < y < 78:
                a = 255
            px[x, y] = (r, g, b, a)
    d = ImageDraw.Draw(im)
    d.rounded_rectangle((24, 24, 72, 72), radius=12, fill=(255, 140, 40, 255), outline=(255, 255, 255, 255), width=3)
    d.ellipse((34, 34, 62, 62), fill=(40, 200, 255, 220), outline=(255, 255, 255, 255), width=2)
    d.line((8, 88, 88, 8), fill=(255, 255, 255, 180), width=2)
    d.text((5, 4), "CC0", fill=(255, 255, 255, 255))
    return im


def make_anim_frames(n: int = 4, w: int = 96, h: int = 96):
    frames = []
    for i in range(n):
        im = Image.new("RGBA", (w, h), (0, 0, 0, 0))
        px = im.load()
        for y in range(h):
            for x in range(w):
                bg = 24 if ((x // 16) + (y // 16)) % 2 else 210
                px[x, y] = (bg, bg, bg, 255)
        d = ImageDraw.Draw(im)
        x0 = 8 + i * 18
        y0 = 12 + (i % 2) * 10
        d.rounded_rectangle((x0, y0, x0 + 34, y0 + 34), radius=8, fill=(255, 64 + i * 40, 32 + i * 24, 220), outline=(255, 255, 255, 255), width=2)
        d.ellipse((54 - i * 10, 50, 88 - i * 10, 84), fill=(48 + i * 32, 180, 255, 200), outline=(0, 0, 0, 255), width=2)
        d.text((4, 76), f"F{i}", fill=(0, 0, 0, 255))
        frames.append(im)
    return frames


base_rgba = make_base_rgba()
base_rgb = base_rgba.convert("RGB")
anim = make_anim_frames()
manifest = {"ok": [], "failed": {}}

static_jobs = [
    ("sample.bmp", base_rgb, {}),
    ("sample.jpg", base_rgb, {"quality": 95}),
    ("sample.png", base_rgba, {}),
    ("sample.tga", base_rgba, {}),
    ("sample.pcx", base_rgb, {}),
    ("sample.qoi", base_rgba, {}),
    ("sample.webp", base_rgba, {"lossless": True, "quality": 100}),
    ("sample.tiff", base_rgba, {}),
    ("sample.dds", base_rgba, {}),
]

for name, image, opts in static_jobs:
    path = OUT / name
    try:
        image.save(path, **opts)
        manifest["ok"].append(name)
    except Exception as exc:
        manifest["failed"][name] = str(exc)

# GIF animation
try:
    gif_frames = [fr.convert("P", palette=Image.Palette.ADAPTIVE) for fr in anim]
    gif_frames[0].save(OUT / "sample.gif", save_all=True, append_images=gif_frames[1:], duration=[80, 120, 160, 200], loop=0, disposal=2)
    manifest["ok"].append("sample.gif")
except Exception as exc:
    manifest["failed"]["sample.gif"] = str(exc)

# APNG animation
try:
    anim[0].save(OUT / "sample.apng.png", save_all=True, append_images=anim[1:], duration=[80, 120, 160, 200], loop=0, disposal=2, blend=0)
    manifest["ok"].append("sample.apng.png")
except Exception as exc:
    manifest["failed"]["sample.apng.png"] = str(exc)

# Animated WEBP
try:
    anim[0].save(OUT / "sample_anim.webp", save_all=True, append_images=anim[1:], duration=[80, 120, 160, 200], loop=0, lossless=True, quality=100)
    manifest["ok"].append("sample_anim.webp")
except Exception as exc:
    manifest["failed"]["sample_anim.webp"] = str(exc)

(OUT / "manifest.json").write_text(json.dumps(manifest, indent=2), encoding="utf-8")
print(json.dumps(manifest, indent=2))
