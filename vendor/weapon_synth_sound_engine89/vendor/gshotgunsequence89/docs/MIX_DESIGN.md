# Pump mix design — v1.1

## Distinct chuecka roles

`chuecka89` has two deliberately separate identities in this sequence.

### Shell insertion

Insertion calls `CH89_PRESET_SHOTGUN_INSERT` or
`CH89_PRESET_SHOTGUN_MULTI_INSERT` directly. It is chuecka-only, darker,
shorter, and not synchronized to gpump because no pump cycle is active.

### Pumping

Pumping combines:

- `gpump89`: staged low/mid mechanical mass — unlock, slide, extractor, rear
  stop, optional ejection, carrier, battery, and lock.
- `chuecka89`: the bright upper harmonic that makes the motion read as
  **chic-chuk** rather than a dull mechanical thud.

Default gains:

- `pump_primary_q15 = 22000`
- `pump_chuecka_q15 = 32767`
- `pump_chuecka_delay_ms = 24`

In the generated `WITH_SHELL` audition, measured RMS is approximately:

- gpump mass stem: `1297.501`
- chuecka harmonic stem: `1614.272`

Therefore chuecka is no longer mixed beneath gpump. Its measured level is about
24 percent higher, while gpump remains perceptually responsible for the lower,
fatter body.

## Automatic chic-to-chuk alignment

The pump-only chuecka gesture has perceptual anchors near 87 ms and 231 ms in
its native timing. v1.1 maps those anchors onto gpump's cycle-relative rear-stop
(40 percent) and battery (90 percent) events, then applies the public fine trim.

For the generated `GPUMP89_PRESET_WITH_SHELL` audition, the strongest paired
transients occur approximately at:

- rear pair: chuecka `187 ms`, gpump `184 ms`
- forward pair: chuecka `408 ms`, gpump `412 ms`

The difference is only 3–4 ms at the main impacts, producing one fused
mechanical event rather than two unrelated layers.

## Pump-only tonal shaping

After creating `CH89_PRESET_SHOTGUN_PUMP`, the orchestrator applies a local
pump-harmonic transform:

- low and low-mid bands are reduced;
- upper-mid and high bands are retained or boosted;
- noise colors are switched to bright;
- chorus and reverb are removed;
- timing follows the selected gpump cycle.

The vendored chuecka library is not globally altered. Insert presets preserve
their original darker tone.
