#!/usr/bin/env python3
"""Generate rawmix Q14 biquad coefficients.

This helper is intentionally outside the C89/no-float core. It computes
normalized biquad coefficients using the standard RBJ cookbook formulas and
prints them as floating-point values plus rawmix-ready Q14 integers.
"""

from __future__ import annotations

import argparse
import math
import sys
from typing import Tuple

Q14_SCALE = 16384.0
Q14_MIN = -32768
Q14_MAX = 32767


def normalize(b0: float, b1: float, b2: float, a0: float, a1: float, a2: float) -> Tuple[float, float, float, float, float]:
    if a0 == 0.0:
        raise ValueError("a0 cannot be zero")
    return b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0


def coeffs_lowpass(fs: float, freq: float, q: float) -> Tuple[float, float, float, float, float]:
    w0 = 2.0 * math.pi * freq / fs
    cosw = math.cos(w0)
    sinw = math.sin(w0)
    alpha = sinw / (2.0 * q)
    return normalize((1.0 - cosw) / 2.0,
                     1.0 - cosw,
                     (1.0 - cosw) / 2.0,
                     1.0 + alpha,
                     -2.0 * cosw,
                     1.0 - alpha)


def coeffs_highpass(fs: float, freq: float, q: float) -> Tuple[float, float, float, float, float]:
    w0 = 2.0 * math.pi * freq / fs
    cosw = math.cos(w0)
    sinw = math.sin(w0)
    alpha = sinw / (2.0 * q)
    return normalize((1.0 + cosw) / 2.0,
                     -(1.0 + cosw),
                     (1.0 + cosw) / 2.0,
                     1.0 + alpha,
                     -2.0 * cosw,
                     1.0 - alpha)


def coeffs_bandpass(fs: float, freq: float, q: float) -> Tuple[float, float, float, float, float]:
    w0 = 2.0 * math.pi * freq / fs
    cosw = math.cos(w0)
    sinw = math.sin(w0)
    alpha = sinw / (2.0 * q)
    return normalize(alpha,
                     0.0,
                     -alpha,
                     1.0 + alpha,
                     -2.0 * cosw,
                     1.0 - alpha)


def coeffs_notch(fs: float, freq: float, q: float) -> Tuple[float, float, float, float, float]:
    w0 = 2.0 * math.pi * freq / fs
    cosw = math.cos(w0)
    sinw = math.sin(w0)
    alpha = sinw / (2.0 * q)
    return normalize(1.0,
                     -2.0 * cosw,
                     1.0,
                     1.0 + alpha,
                     -2.0 * cosw,
                     1.0 - alpha)


def coeffs_allpass(fs: float, freq: float, q: float) -> Tuple[float, float, float, float, float]:
    w0 = 2.0 * math.pi * freq / fs
    cosw = math.cos(w0)
    sinw = math.sin(w0)
    alpha = sinw / (2.0 * q)
    return normalize(1.0 - alpha,
                     -2.0 * cosw,
                     1.0 + alpha,
                     1.0 + alpha,
                     -2.0 * cosw,
                     1.0 - alpha)


def coeffs_peaking(fs: float, freq: float, q: float, gain_db: float) -> Tuple[float, float, float, float, float]:
    w0 = 2.0 * math.pi * freq / fs
    cosw = math.cos(w0)
    sinw = math.sin(w0)
    alpha = sinw / (2.0 * q)
    a = math.pow(10.0, gain_db / 40.0)
    return normalize(1.0 + alpha * a,
                     -2.0 * cosw,
                     1.0 - alpha * a,
                     1.0 + alpha / a,
                     -2.0 * cosw,
                     1.0 - alpha / a)


def to_q14(value: float) -> int:
    return int(round(value * Q14_SCALE))


def build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(description="Generate rawmix rm_biquad_desc Q14 coefficients")
    p.add_argument("kind", choices=["lowpass", "highpass", "bandpass", "notch", "allpass", "peaking"])
    p.add_argument("sample_rate", type=float)
    p.add_argument("frequency_hz", type=float)
    p.add_argument("--q", type=float, default=0.70710678, help="quality factor / resonance control")
    p.add_argument("--gain-db", type=float, default=0.0, help="used only for peaking")
    p.add_argument("--wet-q15", type=int, default=32767)
    p.add_argument("--output-gain-q15", type=int, default=32767)
    return p


def main() -> int:
    parser = build_parser()
    args = parser.parse_args()

    if args.sample_rate <= 0.0 or args.frequency_hz <= 0.0:
        parser.error("sample_rate and frequency_hz must be positive")
    if args.frequency_hz >= (args.sample_rate * 0.5):
        parser.error("frequency_hz must be below Nyquist")
    if args.q <= 0.0:
        parser.error("--q must be positive")

    if args.kind == "lowpass":
        coeffs = coeffs_lowpass(args.sample_rate, args.frequency_hz, args.q)
    elif args.kind == "highpass":
        coeffs = coeffs_highpass(args.sample_rate, args.frequency_hz, args.q)
    elif args.kind == "bandpass":
        coeffs = coeffs_bandpass(args.sample_rate, args.frequency_hz, args.q)
    elif args.kind == "notch":
        coeffs = coeffs_notch(args.sample_rate, args.frequency_hz, args.q)
    elif args.kind == "allpass":
        coeffs = coeffs_allpass(args.sample_rate, args.frequency_hz, args.q)
    else:
        coeffs = coeffs_peaking(args.sample_rate, args.frequency_hz, args.q, args.gain_db)

    q14 = tuple(to_q14(v) for v in coeffs)
    fits_q14 = all(Q14_MIN <= v <= Q14_MAX for v in q14)

    print("float coefficients:")
    print(f"  b0={coeffs[0]: .10f}")
    print(f"  b1={coeffs[1]: .10f}")
    print(f"  b2={coeffs[2]: .10f}")
    print(f"  a1={coeffs[3]: .10f}")
    print(f"  a2={coeffs[4]: .10f}")
    print()
    print("rawmix Q14:")
    print(f"  b0_q14={q14[0]}")
    print(f"  b1_q14={q14[1]}")
    print(f"  b2_q14={q14[2]}")
    print(f"  a1_q14={q14[3]}")
    print(f"  a2_q14={q14[4]}")
    print()
    print("rm_biquad_desc snippet:")
    print("  rm_biquad_desc desc;")
    print(f"  desc.b0_q14 = (rm_s16){q14[0]};")
    print(f"  desc.b1_q14 = (rm_s16){q14[1]};")
    print(f"  desc.b2_q14 = (rm_s16){q14[2]};")
    print(f"  desc.a1_q14 = (rm_s16){q14[3]};")
    print(f"  desc.a2_q14 = (rm_s16){q14[4]};")
    print(f"  desc.wet_q15 = (rm_s16){args.wet_q15};")
    print(f"  desc.output_gain_q15 = (rm_s16){args.output_gain_q15};")

    if not fits_q14:
        print("\nwarning: one or more coefficients exceed rawmix's signed Q14 range [-32768, 32767].", file=sys.stderr)
        print("reduce gain/Q or choose another filter shape.", file=sys.stderr)
        return 2

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
