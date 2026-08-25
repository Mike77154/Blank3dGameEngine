#!/usr/bin/env python3
import ctypes
import os
import shlex
import shutil
import struct
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TMP = ROOT / "tests" / ".tmp_fixtures"
LIB = ROOT / "libpcx.so"

PCX_OK = 0
PCX_ERR_FORMAT = -2
PCX_ERR_UNSUPPORTED = -3
PCX_ERR_EOF = -5


class PCXHeader(ctypes.Structure):
    _fields_ = [
        ("manufacturer", ctypes.c_ubyte),
        ("version", ctypes.c_ubyte),
        ("encoding", ctypes.c_ubyte),
        ("bitsPerPixel", ctypes.c_ubyte),
        ("xMin", ctypes.c_uint16),
        ("yMin", ctypes.c_uint16),
        ("xMax", ctypes.c_uint16),
        ("yMax", ctypes.c_uint16),
        ("hDPI", ctypes.c_uint16),
        ("vDPI", ctypes.c_uint16),
        ("colorMap", ctypes.c_ubyte * 48),
        ("reserved", ctypes.c_ubyte),
        ("nPlanes", ctypes.c_ubyte),
        ("bytesPerLine", ctypes.c_uint16),
        ("paletteInfo", ctypes.c_uint16),
        ("hScreenSize", ctypes.c_uint16),
        ("vScreenSize", ctypes.c_uint16),
        ("filler", ctypes.c_ubyte * 54),
    ]


class PCXPalette(ctypes.Structure):
    _fields_ = [
        ("colors", (ctypes.c_ubyte * 3) * 256),
        ("isValid", ctypes.c_int),
    ]


class PCXImage(ctypes.Structure):
    _fields_ = [
        ("width", ctypes.c_int),
        ("height", ctypes.c_int),
        ("channels", ctypes.c_int),
        ("pixels", ctypes.POINTER(ctypes.c_ubyte)),
    ]


class PCXIndexedImage(ctypes.Structure):
    _fields_ = [
        ("width", ctypes.c_int),
        ("height", ctypes.c_int),
        ("totalBitsPerPixel", ctypes.c_int),
        ("indices", ctypes.POINTER(ctypes.c_ubyte)),
        ("palette", PCXPalette),
    ]


def rle_encode(data: bytes) -> bytes:
    out = bytearray()
    i = 0
    n = len(data)
    while i < n:
        value = data[i]
        run = 1
        while i + run < n and data[i + run] == value and run < 63:
            run += 1
        if run > 1 or value >= 0xC0:
            out.append(0xC0 | run)
            out.append(value)
        else:
            out.append(value)
        i += run
    return bytes(out)


def pack_samples(indices, bits):
    if bits == 8:
        return bytes(indices)
    per_byte = 8 // bits
    mask = (1 << bits) - 1
    out = bytearray()
    for i in range(0, len(indices), per_byte):
        group = indices[i:i + per_byte]
        value = 0
        for j, sample in enumerate(group):
            shift = 8 - bits * (j + 1)
            value |= (sample & mask) << shift
        out.append(value)
    return bytes(out)


def planar_1bpp(indices, nplanes):
    planes = []
    for plane in range(nplanes):
        bits = [((idx >> plane) & 1) for idx in indices]
        planes.append(pack_samples(bits, 1))
    return planes


def build_header(width, height, bits, planes, bytes_per_line, palette16,
                 encoding=1, palette_info=1, version=5, reserved=0):
    hdr = bytearray(128)
    hdr[0] = 0x0A
    hdr[1] = version
    hdr[2] = encoding
    hdr[3] = bits
    struct.pack_into("<HHHH", hdr, 4, 0, 0, width - 1, height - 1)
    struct.pack_into("<HH", hdr, 12, 72, 72)
    flat_palette = []
    for rgb in palette16[:16]:
        flat_palette.extend(rgb)
    flat_palette.extend([0] * (48 - len(flat_palette)))
    hdr[16:64] = bytes(flat_palette[:48])
    hdr[64] = reserved
    hdr[65] = planes
    struct.pack_into("<H", hdr, 66, bytes_per_line)
    struct.pack_into("<H", hdr, 68, palette_info)
    return bytes(hdr)


def write_pcx(path, width, height, bits, planes, rows, palette16, palette256=None,
              encoding=1, palette_info=1, bytes_per_line=None, version=5, reserved=0):
    if bytes_per_line is None:
        min_bpl = (width * bits + 7) // 8
        bytes_per_line = min_bpl + (min_bpl & 1)
    hdr = build_header(width, height, bits, planes, bytes_per_line, palette16,
                       encoding, palette_info, version, reserved)
    payload = bytearray(hdr)
    for row_planes in rows:
        assert len(row_planes) == planes
        for plane_bytes in row_planes:
            if len(plane_bytes) > bytes_per_line:
                raise ValueError(f"row longer than bytes_per_line: {len(plane_bytes)} > {bytes_per_line}")
            if len(plane_bytes) < bytes_per_line:
                plane_bytes = plane_bytes + b"\x00" * (bytes_per_line - len(plane_bytes))
            payload.extend(rle_encode(plane_bytes) if encoding == 1 else plane_bytes)
    if palette256 is not None:
        payload.append(0x0C)
        for rgb in palette256:
            payload.extend(bytes(rgb))
    path.write_bytes(payload)


def compile_lib():
    cc = os.environ.get("PCX_TEST_CC") or os.environ.get("CC") or "gcc"
    cflags = shlex.split(os.environ.get("PCX_TEST_CFLAGS", "-std=c89 -Wall -Wextra -Werror -pedantic"))
    ldflags = shlex.split(os.environ.get("PCX_TEST_LDFLAGS", ""))
    cmd = [
        cc,
        *cflags,
        "-shared",
        "-fPIC",
        "-o",
        str(LIB),
        str(ROOT / "pcx_chunk.c"),
        str(ROOT / "pcx_parser.c"),
        str(ROOT / "pcx_render.c"),
        str(ROOT / "pcx_decoder.c"),
        str(ROOT / "pcx_encoder.c"),
        *ldflags,
    ]
    subprocess.run(cmd, check=True)


def load_lib():
    lib = ctypes.CDLL(str(LIB))
    lib.pcx_load.argtypes = [ctypes.c_char_p, ctypes.POINTER(PCXImage)]
    lib.pcx_load.restype = ctypes.c_int
    lib.pcx_load_strict.argtypes = [ctypes.c_char_p, ctypes.POINTER(PCXImage)]
    lib.pcx_load_strict.restype = ctypes.c_int
    lib.pcx_load_memory.argtypes = [ctypes.c_void_p, ctypes.c_uint32, ctypes.POINTER(PCXImage)]
    lib.pcx_load_memory.restype = ctypes.c_int
    lib.pcx_load_memory_strict.argtypes = [ctypes.c_void_p, ctypes.c_uint32, ctypes.POINTER(PCXImage)]
    lib.pcx_load_memory_strict.restype = ctypes.c_int
    lib.pcx_image_release.argtypes = [ctypes.POINTER(PCXImage)]
    lib.pcx_image_release.restype = None
    lib.pcx_load_indexed.argtypes = [ctypes.c_char_p, ctypes.POINTER(PCXIndexedImage)]
    lib.pcx_load_indexed.restype = ctypes.c_int
    lib.pcx_load_indexed_strict.argtypes = [ctypes.c_char_p, ctypes.POINTER(PCXIndexedImage)]
    lib.pcx_load_indexed_strict.restype = ctypes.c_int
    lib.pcx_load_indexed_memory.argtypes = [ctypes.c_void_p, ctypes.c_uint32, ctypes.POINTER(PCXIndexedImage)]
    lib.pcx_load_indexed_memory.restype = ctypes.c_int
    lib.pcx_load_indexed_memory_strict.argtypes = [ctypes.c_void_p, ctypes.c_uint32, ctypes.POINTER(PCXIndexedImage)]
    lib.pcx_load_indexed_memory_strict.restype = ctypes.c_int
    lib.pcx_indexed_image_release.argtypes = [ctypes.POINTER(PCXIndexedImage)]
    lib.pcx_indexed_image_release.restype = None
    lib.pcx_inspect_file.argtypes = [
        ctypes.c_char_p,
        ctypes.POINTER(PCXHeader),
        ctypes.POINTER(ctypes.c_int),
        ctypes.POINTER(ctypes.c_int),
    ]
    lib.pcx_inspect_file.restype = ctypes.c_int
    lib.pcx_inspect_memory.argtypes = [
        ctypes.c_void_p,
        ctypes.c_uint32,
        ctypes.POINTER(PCXHeader),
        ctypes.POINTER(ctypes.c_int),
        ctypes.POINTER(ctypes.c_int),
    ]
    lib.pcx_inspect_memory.restype = ctypes.c_int
    lib.pcx_inspect_file_strict.argtypes = [
        ctypes.c_char_p,
        ctypes.POINTER(PCXHeader),
        ctypes.POINTER(ctypes.c_int),
        ctypes.POINTER(ctypes.c_int),
    ]
    lib.pcx_inspect_file_strict.restype = ctypes.c_int
    lib.pcx_inspect_memory_strict.argtypes = [
        ctypes.c_void_p,
        ctypes.c_uint32,
        ctypes.POINTER(PCXHeader),
        ctypes.POINTER(ctypes.c_int),
        ctypes.POINTER(ctypes.c_int),
    ]
    lib.pcx_inspect_memory_strict.restype = ctypes.c_int
    return lib


def decode_image(lib, path: Path):
    img = PCXImage()
    result = lib.pcx_load(str(path).encode("utf-8"), ctypes.byref(img))
    if result != PCX_OK:
        return result, None
    try:
        size = img.width * img.height * img.channels
        data = ctypes.string_at(img.pixels, size)
        return result, (img.width, img.height, img.channels, data)
    finally:
        lib.pcx_image_release(ctypes.byref(img))


def decode_indexed_image(lib, path: Path):
    img = PCXIndexedImage()
    result = lib.pcx_load_indexed(str(path).encode("utf-8"), ctypes.byref(img))
    if result != PCX_OK:
        return result, None
    try:
        size = img.width * img.height
        data = ctypes.string_at(img.indices, size)
        palette = [tuple(img.palette.colors[i]) for i in range(256)]
        return result, (img.width, img.height, img.totalBitsPerPixel, data, palette, img.palette.isValid)
    finally:
        lib.pcx_indexed_image_release(ctypes.byref(img))




def decode_image_strict(lib, path: Path):
    img = PCXImage()
    result = lib.pcx_load_strict(str(path).encode("utf-8"), ctypes.byref(img))
    if result != PCX_OK:
        return result, None
    try:
        size = img.width * img.height * img.channels
        data = ctypes.string_at(img.pixels, size)
        return result, (img.width, img.height, img.channels, data)
    finally:
        lib.pcx_image_release(ctypes.byref(img))


def decode_indexed_image_strict(lib, path: Path):
    img = PCXIndexedImage()
    result = lib.pcx_load_indexed_strict(str(path).encode("utf-8"), ctypes.byref(img))
    if result != PCX_OK:
        return result, None
    try:
        size = img.width * img.height
        data = ctypes.string_at(img.indices, size)
        palette = [tuple(img.palette.colors[i]) for i in range(256)]
        return result, (img.width, img.height, img.totalBitsPerPixel, data, palette, img.palette.isValid)
    finally:
        lib.pcx_indexed_image_release(ctypes.byref(img))

def decode_image_memory(lib, raw: bytes):
    img = PCXImage()
    buf = ctypes.create_string_buffer(raw)
    result = lib.pcx_load_memory(buf, len(raw), ctypes.byref(img))
    if result != PCX_OK:
        return result, None
    try:
        size = img.width * img.height * img.channels
        data = ctypes.string_at(img.pixels, size)
        return result, (img.width, img.height, img.channels, data)
    finally:
        lib.pcx_image_release(ctypes.byref(img))




def decode_image_memory_strict(lib, raw: bytes):
    img = PCXImage()
    buf = ctypes.create_string_buffer(raw)
    result = lib.pcx_load_memory_strict(buf, len(raw), ctypes.byref(img))
    if result != PCX_OK:
        return result, None
    try:
        size = img.width * img.height * img.channels
        data = ctypes.string_at(img.pixels, size)
        return result, (img.width, img.height, img.channels, data)
    finally:
        lib.pcx_image_release(ctypes.byref(img))

def decode_indexed_image_memory(lib, raw: bytes):
    img = PCXIndexedImage()
    buf = ctypes.create_string_buffer(raw)
    result = lib.pcx_load_indexed_memory(buf, len(raw), ctypes.byref(img))
    if result != PCX_OK:
        return result, None
    try:
        size = img.width * img.height
        data = ctypes.string_at(img.indices, size)
        palette = [tuple(img.palette.colors[i]) for i in range(256)]
        return result, (img.width, img.height, img.totalBitsPerPixel, data, palette, img.palette.isValid)
    finally:
        lib.pcx_indexed_image_release(ctypes.byref(img))




def decode_indexed_image_memory_strict(lib, raw: bytes):
    img = PCXIndexedImage()
    buf = ctypes.create_string_buffer(raw)
    result = lib.pcx_load_indexed_memory_strict(buf, len(raw), ctypes.byref(img))
    if result != PCX_OK:
        return result, None
    try:
        size = img.width * img.height
        data = ctypes.string_at(img.indices, size)
        palette = [tuple(img.palette.colors[i]) for i in range(256)]
        return result, (img.width, img.height, img.totalBitsPerPixel, data, palette, img.palette.isValid)
    finally:
        lib.pcx_indexed_image_release(ctypes.byref(img))

def inspect_image(lib, path: Path, strict: bool):
    hdr = PCXHeader()
    width = ctypes.c_int()
    height = ctypes.c_int()
    fn = lib.pcx_inspect_file_strict if strict else lib.pcx_inspect_file
    result = fn(
        str(path).encode("utf-8"),
        ctypes.byref(hdr),
        ctypes.byref(width),
        ctypes.byref(height),
    )
    return result, hdr, width.value, height.value


def inspect_memory(lib, raw: bytes, strict: bool):
    hdr = PCXHeader()
    width = ctypes.c_int()
    height = ctypes.c_int()
    buf = ctypes.create_string_buffer(raw)
    fn = lib.pcx_inspect_memory_strict if strict else lib.pcx_inspect_memory
    result = fn(
        buf,
        len(raw),
        ctypes.byref(hdr),
        ctypes.byref(width),
        ctypes.byref(height),
    )
    return result, hdr, width.value, height.value


def rgb_from_indices(rows, palette):
    out = bytearray()
    for row in rows:
        for idx in row:
            out.extend(bytes(palette[idx]))
    return bytes(out)


def build_fixtures(tmp: Path):
    palette16 = [
        (0, 0, 0),
        (255, 255, 255),
        (255, 0, 0),
        (0, 255, 0),
        (0, 0, 255),
        (255, 255, 0),
        (255, 0, 255),
        (0, 255, 255),
        (128, 128, 128),
        (64, 64, 64),
        (192, 64, 0),
        (64, 192, 0),
        (0, 64, 192),
        (200, 100, 50),
        (50, 100, 200),
        (255, 128, 0),
    ]
    palette256 = [(i, 255 - i, (i * 7) & 0xFF) for i in range(256)]
    gray_palette = [(i, i, i) for i in range(256)]

    fixtures = []

    mono_rows = [[0, 1, 0, 1, 0, 1, 0, 1], [1, 1, 1, 1, 0, 0, 0, 0]]
    mono_plane_rows = [[pack_samples(row, 1)] for row in mono_rows]
    p = tmp / "mono_1bpp.pcx"
    write_pcx(p, 8, 2, 1, 1, mono_plane_rows, palette16)
    fixtures.append((p, rgb_from_indices(mono_rows, palette16), PCX_OK))

    packed2_rows = [[0, 1, 2, 3], [3, 2, 1, 0]]
    packed2_plane_rows = [[pack_samples(row, 2)] for row in packed2_rows]
    p = tmp / "packed_2bpp.pcx"
    write_pcx(p, 4, 2, 2, 1, packed2_plane_rows, palette16)
    fixtures.append((p, rgb_from_indices(packed2_rows, palette16), PCX_OK))

    packed4_rows = [[0, 1, 2, 3, 15]]
    packed4_plane_rows = [[pack_samples(row, 4) + b"\x00"] for row in packed4_rows]
    p = tmp / "packed_4bpp.pcx"
    write_pcx(p, 5, 1, 4, 1, packed4_plane_rows, palette16, bytes_per_line=4)
    fixtures.append((p, rgb_from_indices(packed4_rows, palette16), PCX_OK))

    planar4_rows = [[0, 1, 2, 3, 4, 5, 6, 15]]
    planar4_plane_rows = [[plane for plane in planar_1bpp(row, 4)] for row in planar4_rows]
    p = tmp / "planar_4x1bpp.pcx"
    write_pcx(p, 8, 1, 1, 4, planar4_plane_rows, palette16)
    fixtures.append((p, rgb_from_indices(planar4_rows, palette16), PCX_OK))

    idx8_rows = [[0, 1, 2, 3], [128, 129, 130, 131]]
    idx8_plane_rows = [[pack_samples(row, 8)] for row in idx8_rows]
    p = tmp / "indexed_8bpp.pcx"
    write_pcx(p, 4, 2, 8, 1, idx8_plane_rows, palette16, palette256=palette256, bytes_per_line=4)
    fixtures.append((p, rgb_from_indices(idx8_rows, palette256), PCX_OK))

    gray_rows = [[0, 128, 255, 64]]
    gray_plane_rows = [[pack_samples(row, 8)] for row in gray_rows]
    p = tmp / "gray_hint_8bpp_no_palette.pcx"
    write_pcx(p, 4, 1, 8, 1, gray_plane_rows, palette16, palette256=None, palette_info=2, bytes_per_line=4)
    fixtures.append((p, rgb_from_indices(gray_rows, gray_palette), PCX_OK))

    raw8_rows = [[7, 8, 9]]
    raw8_plane_rows = [[pack_samples(row, 8)] for row in raw8_rows]
    p = tmp / "raw_8bpp.pcx"
    write_pcx(p, 3, 1, 8, 1, raw8_plane_rows, palette16, palette256=palette256, encoding=0, bytes_per_line=3)
    fixtures.append((p, rgb_from_indices(raw8_rows, palette256), PCX_OK))

    false_palette_row = [12] + [i & 0xFF for i in range(768)]
    false_palette_plane_rows = [[pack_samples(false_palette_row, 8)]]
    p = tmp / "false_palette_from_eof.pcx"
    write_pcx(p, 769, 1, 8, 1, false_palette_plane_rows, palette16, palette256=None,
              encoding=0, palette_info=2, bytes_per_line=769)
    fixtures.append((p, rgb_from_indices([false_palette_row], gray_palette), PCX_OK))

    rgb_rows = [
        [(10, 20, 30), (40, 50, 60), (70, 80, 90)],
        [(90, 80, 70), (60, 50, 40), (30, 20, 10)],
    ]
    rgb_plane_rows = []
    for row in rgb_rows:
        r = bytes(pixel[0] for pixel in row)
        g = bytes(pixel[1] for pixel in row)
        b = bytes(pixel[2] for pixel in row)
        rgb_plane_rows.append([r + b"\x00", g + b"\x00", b + b"\x00"])
    p = tmp / "rgb24.pcx"
    write_pcx(p, 3, 2, 8, 3, rgb_plane_rows, palette16, bytes_per_line=4)
    expected = bytearray()
    for row in rgb_rows:
        for pixel in row:
            expected.extend(bytes(pixel))
    fixtures.append((p, bytes(expected), PCX_OK))

    bad = tmp / "bad_rle_overflow.pcx"
    hdr = build_header(1, 1, 8, 1, 1, palette16)
    bad.write_bytes(hdr + bytes([0xC2, 0x05]))
    fixtures.append((bad, None, PCX_ERR_FORMAT))

    bad = tmp / "bad_rle_truncated_value.pcx"
    hdr = build_header(1, 1, 8, 1, 1, palette16)
    bad.write_bytes(hdr + bytes([0xC1]))
    fixtures.append((bad, None, PCX_ERR_EOF))

    reserved_bad = tmp / "reserved_nonzero_strict.pcx"
    write_pcx(reserved_bad, 2, 1, 1, 1, [[pack_samples([0, 1], 1)]], palette16, reserved=7)

    bad_version_idx8 = tmp / "indexed_8bpp_version4.pcx"
    write_pcx(bad_version_idx8, 4, 1, 8, 1, [[pack_samples([0, 1, 2, 3], 8)]], palette16, palette256=palette256, bytes_per_line=4, version=4)

    bad_version_rgb24 = tmp / "rgb24_version4.pcx"
    write_pcx(bad_version_rgb24, 3, 1, 8, 3, [[bytes([10, 20, 30]) + b"\x00", bytes([40, 50, 60]) + b"\x00", bytes([70, 80, 90]) + b"\x00"]], palette16, bytes_per_line=4, version=4)

    return fixtures


def main():
    if TMP.exists():
        shutil.rmtree(TMP)
    TMP.mkdir(parents=True)

    compile_lib()
    lib = load_lib()
    fixtures = build_fixtures(TMP)

    failures = []
    for path, expected, expected_result in fixtures:
        result, decoded = decode_image(lib, path)
        if result != expected_result:
            failures.append(f"{path.name}: esperado result {expected_result}, salió {result}")
            continue
        if expected_result == PCX_OK:
            if decoded is None:
                failures.append(f"{path.name}: decode nulo")
                continue
            width, height, channels, data = decoded
            if channels != 3:
                failures.append(f"{path.name}: channels {channels} != 3")
                continue
            if data != expected:
                failures.append(f"{path.name}: RGB inesperado")

    raw_path = TMP / "raw_8bpp.pcx"
    strict_rc, _, _, _ = inspect_image(lib, raw_path, True)
    if strict_rc != PCX_ERR_UNSUPPORTED:
        failures.append(f"raw_8bpp strict: esperado {PCX_ERR_UNSUPPORTED}, salió {strict_rc}")

    tolerant_rc, _, width, height = inspect_image(lib, raw_path, False)
    if not (tolerant_rc == PCX_OK and width == 3 and height == 1):
        failures.append("raw_8bpp tolerant: inspección inesperada")

    reserved_path = TMP / "reserved_nonzero_strict.pcx"
    strict_reserved_rc, _, _, _ = inspect_image(lib, reserved_path, True)
    if strict_reserved_rc != PCX_ERR_FORMAT:
        failures.append(f"reserved_nonzero strict: esperado {PCX_ERR_FORMAT}, salió {strict_reserved_rc}")

    bad_version_idx8_path = TMP / "indexed_8bpp_version4.pcx"
    strict_bad_version_idx8_rc, _, _, _ = inspect_image(lib, bad_version_idx8_path, True)
    if strict_bad_version_idx8_rc != PCX_ERR_UNSUPPORTED:
        failures.append(f"indexed_8bpp version4 strict: esperado {PCX_ERR_UNSUPPORTED}, salió {strict_bad_version_idx8_rc}")
    tolerant_bad_version_idx8_rc, _, bad_w, bad_h = inspect_image(lib, bad_version_idx8_path, False)
    if tolerant_bad_version_idx8_rc != PCX_OK or (bad_w, bad_h) != (4, 1):
        failures.append("indexed_8bpp version4 tolerant: inspección inesperada")

    bad_version_rgb24_path = TMP / "rgb24_version4.pcx"
    strict_bad_version_rgb24_rc, _, _, _ = inspect_image(lib, bad_version_rgb24_path, True)
    if strict_bad_version_rgb24_rc != PCX_ERR_UNSUPPORTED:
        failures.append(f"rgb24 version4 strict: esperado {PCX_ERR_UNSUPPORTED}, salió {strict_bad_version_rgb24_rc}")

    idx8_path = TMP / "indexed_8bpp.pcx"
    indexed_rc, indexed = decode_indexed_image(lib, idx8_path)
    if indexed_rc != PCX_OK:
        failures.append(f"indexed_8bpp indexed decode: esperado {PCX_OK}, salió {indexed_rc}")
    else:
        width_i, height_i, total_bits, data_i, palette_i, palette_valid = indexed
        if (width_i, height_i, total_bits) != (4, 2, 8):
            failures.append(f"indexed_8bpp metadata inesperada: {(width_i, height_i, total_bits)}")
        if data_i != bytes([0, 1, 2, 3, 128, 129, 130, 131]):
            failures.append("indexed_8bpp indices inesperados")
        if palette_valid != 1 or palette_i[128] != (128, 127, 128):
            failures.append("indexed_8bpp palette inesperada")

    rgb24_path = TMP / "rgb24.pcx"
    indexed_rgb24_rc, _ = decode_indexed_image(lib, rgb24_path)
    if indexed_rgb24_rc != PCX_ERR_UNSUPPORTED:
        failures.append(f"rgb24 indexed decode: esperado {PCX_ERR_UNSUPPORTED}, salió {indexed_rgb24_rc}")

    idx8_raw = idx8_path.read_bytes()
    mem_rc, mem_decoded = decode_image_memory(lib, idx8_raw)
    if mem_rc != PCX_OK or mem_decoded is None or mem_decoded[3] != rgb_from_indices([[0, 1, 2, 3], [128, 129, 130, 131]], [(i, 255 - i, (i * 7) & 0xFF) for i in range(256)]):
        failures.append("indexed_8bpp memory decode inesperado")

    mem_idx_rc, mem_idx = decode_indexed_image_memory(lib, idx8_raw)
    if mem_idx_rc != PCX_OK or mem_idx is None or mem_idx[3] != bytes([0, 1, 2, 3, 128, 129, 130, 131]):
        failures.append("indexed_8bpp indexed memory decode inesperado")

    mem_inspect_rc, _, mem_w, mem_h = inspect_memory(lib, idx8_raw, False)
    if mem_inspect_rc != PCX_OK or (mem_w, mem_h) != (4, 2):
        failures.append("indexed_8bpp memory inspect inesperado")

    raw_mem_rc, _, _, _ = inspect_memory(lib, raw_path.read_bytes(), True)
    if raw_mem_rc != PCX_ERR_UNSUPPORTED:
        failures.append(f"raw_8bpp strict memory: esperado {PCX_ERR_UNSUPPORTED}, salió {raw_mem_rc}")

    if failures:
        print("FALLOS:")
        for failure in failures:
            print(" -", failure)
        return 1

    print(f"OK: {len(fixtures)} casos validados + strict mode + strict decode API + indexed API + memory API")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
