# Midpoint KLEK design

The pump is now a three-landmark gesture:

```text
rear contact             midpoint articulation            battery contact
CHIC / CHUK  -----------  KLEK + low tik  ---------------- CHIC / CHUK
0.40 cycle                0.625 cycle                       0.90 cycle
```

## Cheap synthesis

`gklek89` uses:

- a one-sample impact and negative rebound;
- seeded white-noise particles;
- two one-pole states to derive band/high energy;
- two tiny feedback combs for receiver/cavity modes;
- independent fixed-point noise and ring decay.

The sequence uses `GKL89_PRESET_CARRIER_KLEK` after fired-shell pumps and
`GKL89_PRESET_REAR_STOP` for the initial heavy pump. Eleven milliseconds after
the KLEK, `gweaponfoley89` contributes a low-gain fast shotgun-safety particle
as the trailing tik.

## Default bus gains

- gpump mass: 22000 Q15
- chuecka harmonic: 32767 Q15
- midpoint KLEK: 26400 Q15
- trailing Foley tik: 8200 Q15
