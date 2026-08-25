from __future__ import annotations

from pathlib import Path
from PIL import Image, ImageDraw, ImageFont
import subprocess
import math

ROOT = Path(__file__).resolve().parents[1]
INPUT = ROOT / "demo_assets" / "input"
DUMP = ROOT / "demo_assets" / "dumped"
REPORT = ROOT / "demo_assets" / "report"
CLI = ROOT / "build" / "demo_cli"
DUMP.mkdir(parents=True, exist_ok=True)
REPORT.mkdir(parents=True, exist_ok=True)
FONT = ImageFont.load_default()


def read_pam_rgba(path: Path) -> Image.Image:
    data = path.read_bytes()
    marker = b"ENDHDR\n"
    idx = data.find(marker)
    if idx < 0:
        raise ValueError(f"bad PAM header: {path}")
    header = data[:idx + len(marker)].decode("ascii", errors="strict")
    width = height = depth = maxval = None
    for line in header.splitlines():
        parts = line.strip().split()
        if len(parts) == 2:
            if parts[0] == "WIDTH":
                width = int(parts[1])
            elif parts[0] == "HEIGHT":
                height = int(parts[1])
            elif parts[0] == "DEPTH":
                depth = int(parts[1])
            elif parts[0] == "MAXVAL":
                maxval = int(parts[1])
    if width is None or height is None or depth != 4 or maxval != 255:
        raise ValueError(f"unsupported PAM: {path}")
    body = data[idx + len(marker):]
    need = width * height * 4
    if len(body) < need:
        raise ValueError(f"truncated PAM: {path}")
    return Image.frombytes("RGBA", (width, height), body[:need])



def parse_kv(text: str):
    out = {}
    for line in text.splitlines():
        if "=" in line:
            k, v = line.split("=", 1)
            out[k.strip()] = v.strip()
    return out


def run_dump(path: Path):
    prefix = DUMP / path.name.replace(".", "_")
    cmd = [str(CLI), str(path), "--dump-prefix", str(prefix)]
    proc = subprocess.run(cmd, capture_output=True, text=True, check=False)
    text = proc.stdout + ("\nSTDERR\n" + proc.stderr if proc.stderr else "")
    meta = parse_kv(proc.stdout)
    frames = sorted(DUMP.glob(prefix.name + "_*.pam"))
    return proc.returncode, text, meta, frames


def make_tile(img: Image.Image, caption: str, tile_w: int = 180, tile_h: int = 180) -> Image.Image:
    panel = Image.new("RGBA", (tile_w, tile_h), (250, 250, 250, 255))
    draw = ImageDraw.Draw(panel)
    panel.paste((236, 236, 236, 255), (0, 0, tile_w, tile_h))
    inner_h = tile_h - 28
    fit = img.copy().convert("RGBA")
    fit.thumbnail((tile_w - 12, inner_h - 12))
    x = (tile_w - fit.width) // 2
    y = (inner_h - fit.height) // 2 + 4
    checker = Image.new("RGBA", (fit.width, fit.height), (0, 0, 0, 0))
    cpx = checker.load()
    for yy in range(fit.height):
        for xx in range(fit.width):
            v = 210 if ((xx // 8) + (yy // 8)) % 2 == 0 else 180
            cpx[xx, yy] = (v, v, v, 255)
    panel.alpha_composite(checker, (x, y))
    panel.alpha_composite(fit, (x, y))
    draw.rectangle((0, tile_h - 26, tile_w - 1, tile_h - 1), fill=(32, 32, 32, 255))
    draw.text((6, tile_h - 21), caption, fill=(255, 255, 255, 255), font=FONT)
    draw.rectangle((0, 0, tile_w - 1, tile_h - 1), outline=(120, 120, 120, 255), width=1)
    return panel


def make_strip(frames: list[Image.Image], caption: str) -> Image.Image:
    thumbs = []
    for im in frames:
        cp = im.copy().convert("RGBA")
        cp.thumbnail((112, 112))
        thumbs.append(cp)
    strip_w = max(200, 8 + sum(im.width + 8 for im in thumbs))
    strip_h = 148
    panel = Image.new("RGBA", (strip_w, strip_h), (246, 246, 246, 255))
    draw = ImageDraw.Draw(panel)
    x = 8
    for im in thumbs:
        y = 10 + (96 - im.height) // 2
        checker = Image.new("RGBA", (im.width, im.height), (0, 0, 0, 0))
        cpx = checker.load()
        for yy in range(im.height):
            for xx in range(im.width):
                v = 210 if ((xx // 8) + (yy // 8)) % 2 == 0 else 180
                cpx[xx, yy] = (v, v, v, 255)
        panel.alpha_composite(checker, (x, y))
        panel.alpha_composite(im, (x, y))
        draw.rectangle((x - 1, y - 1, x + im.width, y + im.height), outline=(120, 120, 120, 255), width=1)
        x += im.width + 8
    draw.rectangle((0, strip_h - 28, strip_w - 1, strip_h - 1), fill=(40, 40, 40, 255))
    draw.text((8, strip_h - 22), caption, fill=(255, 255, 255, 255), font=FONT)
    draw.rectangle((0, 0, strip_w - 1, strip_h - 1), outline=(120, 120, 120, 255), width=1)
    return panel


def compose_grid(tiles: list[Image.Image], title: str, cols: int = 3) -> Image.Image:
    if not tiles:
        img = Image.new("RGBA", (640, 120), (255, 255, 255, 255))
        d = ImageDraw.Draw(img)
        d.text((16, 16), title + " (sin datos)", fill=(0, 0, 0, 255), font=FONT)
        return img
    cols = max(1, cols)
    rows = int(math.ceil(len(tiles) / float(cols)))
    tile_w = max(t.width for t in tiles)
    tile_h = max(t.height for t in tiles)
    pad = 12
    title_h = 34
    out = Image.new("RGBA", (pad + cols * (tile_w + pad), title_h + pad + rows * (tile_h + pad)), (255, 255, 255, 255))
    d = ImageDraw.Draw(out)
    d.text((12, 10), title, fill=(0, 0, 0, 255), font=FONT)
    for i, tile in enumerate(tiles):
        row = i // cols
        col = i % cols
        x = pad + col * (tile_w + pad)
        y = title_h + pad + row * (tile_h + pad)
        out.alpha_composite(tile, (x, y))
    return out


all_logs = []
static_tiles = []
anim_tiles = []
for path in sorted(INPUT.iterdir()):
    if path.is_dir():
        continue
    rc, text, meta, frame_paths = run_dump(path)
    all_logs.append("=== %s ===\n%s\n" % (path.name, text.strip()))
    if rc != 0 or not frame_paths:
        continue
    frames = [read_pam_rgba(fp) for fp in frame_paths]
    fmt = meta.get("format", path.suffix.upper().lstrip("."))
    is_anim = int(meta.get("animated", "0") or 0)
    if is_anim or len(frames) > 1:
        caption = f"{path.name}  |  {fmt}  |  {len(frames)} frames"
        anim_tiles.append(make_strip(frames, caption))
    else:
        caption = f"{path.name}  |  {fmt}"
        static_tiles.append(make_tile(frames[0], caption))

static_report = compose_grid(static_tiles, "imgcc0 demo: formatos estáticos decodificados por el wrapper", cols=3)
anim_report = compose_grid(anim_tiles, "imgcc0 demo: animaciones decodificadas por el wrapper", cols=1)
static_report.save(REPORT / "report_static.png")
anim_report.save(REPORT / "report_anim.png")
(REPORT / "report_metadata.txt").write_text("\n\n".join(all_logs), encoding="utf-8")
print("saved", REPORT / "report_static.png")
print("saved", REPORT / "report_anim.png")
print("saved", REPORT / "report_metadata.txt")
