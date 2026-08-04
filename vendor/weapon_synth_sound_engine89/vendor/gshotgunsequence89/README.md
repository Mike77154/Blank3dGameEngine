# gshotgunsequence89 v1.3 — midpoint KLEK pass

Procedural C89/fixed-point shotgun sequence.

## Pump identity

```text
CHIC  -- rail/body --  KLEK-tik  -- rail/body --  CHUK
chuecka + gpump        gklek89 + Foley             chuecka + gpump
```

- `gpump89`: low/mid mass and rail movement.
- `chuecka89`: equally present bright harmonic.
- `gklek89`: dry carrier/receiver midpoint impact at 62.5% of the cycle.
- `gweaponfoley89`: low trailing `tik`, 11 ms after the KLEK.

Shell insertion remains darker `chuecka89` alone, so loading and pumping retain different tonal vocabularies.

## Constraints

Strict C89, fixed point/integer only, no heap allocation, no float/double in the DSP core.

## Audio auditions

- `00_shotgun_sequence_with_mid_klek_tik.wav`
- `01_pump_full_chic_klek_tik_chuk.wav`
- `02_pump_v1_1_without_midpoint.wav`
- `03_gklek89_midpoint_stem.wav`
- `04_foley_tik_midpoint_stem.wav`
- `05_AB_without_midpoint_then_klek_plus_tik.wav`
- `06_klek_then_tik_then_full_pump.wav`


## v1.3 triple pump layer

The complete pump now layers `gpump89` (structural stages), `chuecka89` (bright articulation), and `shotpumpkin89` (continuous rail friction/chatter). The shotpumpkin pull/gap/pump timing is stretched to gpump rear-stop, forward-friction, and lock anchors. Set `pump_shotpumpkin_q15` to zero for the legacy pair.
