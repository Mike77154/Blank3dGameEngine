# Motion89 v3.15.1 — MinGW32 Q16.16 attack-vector fix

## Symptom

`air_lunger_enemy` and `ground_lancer_enemy` entered their attack animation but
mostly hopped or trembled on their own vertical axis. They did not commit toward
the player, so contact and normal attack completion rarely occurred.

## Root cause

The shared failure was inside the two independent fixed-point vendors:

```text
vendor/gairlunge89/src/gairlunge89.c      gal_fx_div / gal_fx_mul
vendor/ggroundlance89/src/ggroundlance89.c ggl_fx_div / ggl_fx_mul
```

Their scalar is Q16.16 `signed long`. On MinGW32, `long` is 32 bits. The former
division reconstructed the fractional result with this intermediate:

```c
(rem * 65536L) / b
```

For a representative 14-unit X difference, `rem` is `14 * 65536`. Multiplying
again by 65536 exceeds signed 32-bit range; some integer-aligned differences
wrap exactly to zero. Vector normalization therefore produced approximately:

```text
wanted direction:  X != 0, Y small, Z != 0
broken direction:  X == 0, Y small, Z == 0
```

The vertical jump/hop component was still generated, making the enemies look as
though they were repeatedly bouncing in place. FPIL and transform ownership
were functioning; they were receiving a damaged direction vector.

Linux x86-64 did not reveal the defect because its default `long` is 64 bits.

## Repair

Both vendors now use C89-safe Q16.16 arithmetic that does not require a wider
signed integer:

- sign is separated from an unsigned magnitude, including `LONG_MIN`;
- multiplication is decomposed into whole/16-bit-fraction terms;
- division emits 16 fractional bits by bounded remainder doubling;
- overflow saturates at the scalar limit instead of wrapping;
- no heap, floating point, `long long`, compiler intrinsic or platform API was
  introduced.

## Regression

`tests/test_motion_q16_win32.c` intentionally defines both vendor scalars as
`signed int`. This exercises a 32-bit Q16.16 path on build hosts where `long`
may be 64 bits. It checks the exact integer-aligned multiply/divide pattern and
verifies that normalization retains non-zero, correctly signed X/Z components.

Run:

```bash
make test-motion-q16-win32
make test-motion-attacks
```

For a clean native build under MSYS2 MinGW32:

```bash
make clean
make
./blank3d.exe
```
