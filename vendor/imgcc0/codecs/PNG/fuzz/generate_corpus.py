#!/usr/bin/env python3
import base64
import os
import shutil
import struct
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
FUZZ = ROOT / 'fuzz'
CORPUS = FUZZ / 'corpus'

PNG_SIG = b'\x89PNG\r\n\x1a\n'
VALID_SRGB_ICCP_COMPRESSED = base64.b64decode(
    b'eJx1kbtLw1AUxn+2SqUqHRQRccig4mBBFMRR69ClSKkVfC1tTFqhj5C0SHEVXBwKDqKLr8H/QFfB'
    b'VUEQFEHE2dHXIiWe2woVaW+4OT++e7/DyRfwRDJ61mmdhmyuYMfCIW1xaVnzveLFD/QyldAdayYa'
    b'jdB0fd3ToupdUPVqfq/h6lgzHB1a2oUndcsuCMs0RDYKluId4R49nVgTPhIetWVA4WulJ2v8ojhV'
    b'4w/Fdjw2Cx7VU0v94eQf1tN2VnhEeDCbKeq/86gv6TRyC/NS+2UP4BAjTAiNJEXWyVAgKDUnmTX2'
    b'jVV9c+TFo8vbooQtjhRp8Y6KWpSuhlRTdEOeDCWV+/88HXNivNa9MwRtz677PgS+XaiUXff72HUr'
    b'J+B9gstc3Z+XnKY+RS/XtcFDCGzB+VVdS+7BxTb0PVoJO1GVvLI9pglvZ9C1BN234F+pZfV7zukD'
    b'xDflF93A/gEMy/3A6g/+3mgJ'
)


def be32(n: int) -> bytes:
    return struct.pack('>I', n & 0xFFFFFFFF)


def be16(n: int) -> bytes:
    return struct.pack('>H', n & 0xFFFF)


def chunk(typ: bytes, data: bytes, crc_flip: bool = False) -> bytes:
    crc = zlib.crc32(typ)
    crc = zlib.crc32(data, crc) & 0xFFFFFFFF
    if crc_flip:
        crc ^= 0x00000001
    return be32(len(data)) + typ + data + be32(crc)


def rgba_pixels(width: int, height: int, seed: int = 0) -> bytes:
    out = bytearray()
    for y in range(height):
        for x in range(width):
            out.extend([
                (x * 31 + y * 17 + seed) & 0xFF,
                (x * 13 + y * 57 + seed * 3) & 0xFF,
                (x * 7 + y * 23 + seed * 5) & 0xFF,
                (255 - (x * 9 + y * 11 + seed)) & 0xFF,
            ])
    return bytes(out)


def scanlines_rgba(width: int, height: int, seed: int = 0) -> bytes:
    raw = bytearray()
    pix = rgba_pixels(width, height, seed)
    rowbytes = width * 4
    for y in range(height):
        raw.append(0)
        raw.extend(pix[y * rowbytes:(y + 1) * rowbytes])
    return bytes(raw)


def scanlines_gray(width: int, height: int, seed: int = 0) -> bytes:
    raw = bytearray()
    for y in range(height):
        raw.append(0)
        for x in range(width):
            raw.append((x * 19 + y * 37 + seed) & 0xFF)
    return bytes(raw)


def scanlines_indexed(width: int, height: int, palette_entries: int) -> bytes:
    raw = bytearray()
    for y in range(height):
        raw.append(0)
        for x in range(width):
            raw.append((x + y * 3) % palette_entries)
    return bytes(raw)


def make_png_rgba(width: int, height: int, *, interlace: int = 0, seed: int = 0,
                  extra_pre=None, extra_post=None, bad_crc: bool = False,
                  text: bool = False, iccp: bool = False, phys: bool = False,
                  unknown_pre=None, unknown_post=None) -> bytes:
    extra_pre = list(extra_pre or [])
    extra_post = list(extra_post or [])
    unknown_pre = list(unknown_pre or [])
    unknown_post = list(unknown_post or [])
    ihdr = struct.pack('>IIBBBBB', width, height, 8, 6, 0, 0, interlace)
    raw = scanlines_rgba(width, height, seed)
    idat = zlib.compress(raw, 6)
    out = bytearray(PNG_SIG)
    out.extend(chunk(b'IHDR', ihdr))
    if phys:
        out.extend(chunk(b'pHYs', be32(3780) + be32(3780) + b'\x01'))
    if text:
        out.extend(chunk(b'tEXt', b'Comment\x00fuzz-rgba-seed'))
        out.extend(chunk(b'iTXt', b'Label\x00\x00\x00en-US\x00Label\x00hello'))
    if iccp:
        prof = zlib.compress(b'fake-icc-profile-data' * 2)
        out.extend(chunk(b'iCCP', b'fuzzicc\x00\x00' + prof))
    for typ, data in unknown_pre:
        out.extend(chunk(typ, data))
    for blob in extra_pre:
        out.extend(blob)
    out.extend(chunk(b'IDAT', idat, crc_flip=bad_crc))
    for blob in extra_post:
        out.extend(blob)
    for typ, data in unknown_post:
        out.extend(chunk(typ, data))
    out.extend(chunk(b'IEND', b''))
    return bytes(out)


def make_png_gray(width: int, height: int, seed: int = 0) -> bytes:
    ihdr = struct.pack('>IIBBBBB', width, height, 8, 0, 0, 0, 0)
    raw = scanlines_gray(width, height, seed)
    out = bytearray(PNG_SIG)
    out.extend(chunk(b'IHDR', ihdr))
    out.extend(chunk(b'IDAT', zlib.compress(raw, 6)))
    out.extend(chunk(b'IEND', b''))
    return bytes(out)


def make_png_palette(width: int, height: int, *, include_hist: bool = False) -> bytes:
    palette_entries = 4
    palette = bytes([
        255, 0, 0,
        0, 255, 0,
        0, 0, 255,
        255, 255, 0,
    ])
    trns = bytes([255, 180, 120, 60])
    hist = b''.join(be16(v) for v in (1, 3, 7, 11))
    splt = b'fuzz-splt\x00' + b'\x08' + bytes([
        255, 0, 0, 255]) + be16(1) + bytes([
        0, 255, 0, 128]) + be16(2)
    txt = chunk(b'tEXt', b'Title\x00palette')
    ztxt = chunk(b'zTXt', b'Comment\x00\x00' + zlib.compress(b'zip-text', 6))
    iccp = chunk(b'iCCP', b'sRGB IEC61966-2.1\x00\x00' + VALID_SRGB_ICCP_COMPRESSED)
    ihdr = struct.pack('>IIBBBBB', width, height, 8, 3, 0, 0, 0)
    raw = scanlines_indexed(width, height, palette_entries)
    out = bytearray(PNG_SIG)
    out.extend(chunk(b'IHDR', ihdr))
    out.extend(iccp)
    out.extend(chunk(b'PLTE', palette))
    out.extend(txt)
    out.extend(ztxt)
    out.extend(chunk(b'tRNS', trns))
    if include_hist:
        out.extend(chunk(b'hIST', hist))
    out.extend(chunk(b'sPLT', splt))
    out.extend(chunk(b'IDAT', zlib.compress(raw, 6)))
    out.extend(chunk(b'IEND', b''))
    return bytes(out)


def make_apng(frame_count: int = 2, *, bad_seq: bool = False, bad_declared: bool = False) -> bytes:
    width, height = 3, 2
    ihdr = struct.pack('>IIBBBBB', width, height, 8, 6, 0, 0, 0)
    seq = 0
    out = bytearray(PNG_SIG)
    out.extend(chunk(b'IHDR', ihdr))
    declared = frame_count + 1 if bad_declared else frame_count
    out.extend(chunk(b'acTL', be32(declared) + be32(0)))
    out.extend(chunk(b'fcTL', be32(seq) + be32(width) + be32(height) + be32(0) + be32(0) + be16(1) + be16(10) + b'\x00\x00'))
    seq += 1
    raw0 = scanlines_rgba(width, height, 1)
    out.extend(chunk(b'IDAT', zlib.compress(raw0, 6)))
    for frame_index in range(1, frame_count):
        use_seq = seq + 1 if (bad_seq and frame_index == frame_count - 1) else seq
        out.extend(chunk(b'fcTL', be32(use_seq) + be32(width) + be32(height) + be32(0) + be32(0) + be16(frame_index) + be16(10) + b'\x00\x00'))
        seq += 1
        raw = scanlines_rgba(width, height, frame_index + 1)
        out.extend(chunk(b'fdAT', be32(seq) + zlib.compress(raw, 6)))
        seq += 1
    out.extend(chunk(b'IEND', b''))
    return bytes(out)


def make_huge_ihdr() -> bytes:
    out = bytearray(PNG_SIG)
    out.extend(chunk(b'IHDR', struct.pack('>IIBBBBB', 0x7FFFFFFF, 0x7FFFFFFF, 8, 6, 0, 0, 0)))
    out.extend(chunk(b'IEND', b''))
    return bytes(out)


def make_out_of_order() -> bytes:
    raw = scanlines_rgba(2, 2, 9)
    out = bytearray(PNG_SIG)
    out.extend(chunk(b'IDAT', zlib.compress(raw, 6)))
    out.extend(chunk(b'IHDR', struct.pack('>IIBBBBB', 2, 2, 8, 6, 0, 0, 0)))
    out.extend(chunk(b'IEND', b''))
    return bytes(out)


def make_large_chunk_header() -> bytes:
    out = bytearray(PNG_SIG)
    out.extend(chunk(b'IHDR', struct.pack('>IIBBBBB', 1, 1, 8, 6, 0, 0, 0)))
    out.extend(be32(0x40000000))
    out.extend(b'vpAg')
    out.extend(b'')
    return bytes(out)


def write(path: Path, data: bytes):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(data)


def seed_raw_blobs():
    blobs = {}
    blobs['empty.bin'] = b''
    blobs['one-null.bin'] = b'\x00'
    blobs['tiny.bin'] = bytes(range(16))
    blobs['ff-64.bin'] = b'\xff' * 64
    blobs['seq-256.bin'] = bytes((i * 7) & 0xFF for i in range(256))
    blobs['alternating.bin'] = bytes((0xAA, 0x55)) * 128
    blobs['params-512.bin'] = bytes((i * 29 + 3) & 0xFF for i in range(512))
    blobs['wide-1k.bin'] = bytes((i * 13 + 91) & 0xFF for i in range(1024))
    blobs['gradient-2k.bin'] = bytes((i // 8) & 0xFF for i in range(2048))
    blobs['randomish-4k.bin'] = bytes((i * 97 + i // 3) & 0xFF for i in range(4096))
    return blobs


def main():
    if CORPUS.exists():
        shutil.rmtree(CORPUS)
    CORPUS.mkdir(parents=True, exist_ok=True)

    png_unknown = make_png_rgba(
        4, 4, seed=5,
        unknown_pre=[(b'vpAg', b'alpha'), (b'faKe', b'beta')],
        unknown_post=[(b'prIv', b'omega')]
    )
    png_text_iccp = make_png_rgba(3, 5, seed=7, text=True, iccp=True, phys=True)
    png_bad_crc = make_png_rgba(2, 2, seed=11, bad_crc=True)
    png_trunc = make_png_rgba(3, 3, seed=13)[:-9]
    png_many_chunks = make_png_rgba(
        2, 2, seed=1,
        unknown_pre=[(b'vpAg', f'chunk-{i}'.encode()) for i in range(20)]
    )

    decode_files = {
        'valid_rgba.png': make_png_rgba(4, 4, seed=1),
        'valid_gray.png': make_png_gray(5, 3, seed=2),
        'valid_text_iccp.png': png_text_iccp,
        'valid_unknown.png': png_unknown,
        'valid_palette_meta.png': make_png_palette(6, 4),
        'valid_hist_palette.png': make_png_palette(6, 4, include_hist=True),
        'valid_largeish.png': make_png_rgba(32, 16, seed=3),
        'bad_crc.png': png_bad_crc,
        'truncated.png': png_trunc,
        'huge_ihdr.png': make_huge_ihdr(),
        'out_of_order.png': make_out_of_order(),
        'too_many_unknownish.png': png_many_chunks,
        'large_chunk_header.png': make_large_chunk_header(),
    }

    apng_files = {
        'valid_two_frame.apng': make_apng(2),
        'valid_three_frame.apng': make_apng(3),
        'bad_seq.apng': make_apng(3, bad_seq=True),
        'bad_declared.apng': make_apng(2, bad_declared=True),
        'truncated.apng': make_apng(2)[:-7],
    }

    for target in ('png_fuzz_decode_full', 'png_fuzz_decode_incremental'):
        tdir = CORPUS / target
        for name, data in decode_files.items():
            write(tdir / name, data)
        shutil.copy2(ROOT / 'test_adam7_iccp.png', tdir / 'seed_test_adam7_iccp.png')
        shutil.copy2(ROOT / 'test_apng_anim.png', tdir / 'seed_test_apng_anim.png')

    tdir = CORPUS / 'png_fuzz_apng_decode'
    for name, data in {**apng_files, **{'pngish.png': make_png_rgba(2, 2, seed=21)}}.items():
        write(tdir / name, data)
    shutil.copy2(ROOT / 'test_apng_anim.png', tdir / 'seed_test_apng_anim.png')

    raw_targets = [
        'png_fuzz_encode_png',
        'png_fuzz_encode_apng',
        'png_fuzz_unknown_chunks',
        'png_fuzz_convert_formats',
        'png_fuzz_adam7',
        'png_fuzz_metadata',
    ]
    raw_blobs = seed_raw_blobs()
    for target in raw_targets:
        tdir = CORPUS / target
        for name, data in raw_blobs.items():
            write(tdir / name, data)
        # sprinkle a few png/apng samples too; the harnesses treat them as structured bytes,
        # which is still useful coverage for magic values and chunk names.
        write(tdir / 'sample_png.bin', decode_files['valid_unknown.png'])
        write(tdir / 'sample_apng.bin', apng_files['valid_two_frame.apng'])

    print(f'generated corpus under {CORPUS}')


if __name__ == '__main__':
    main()
