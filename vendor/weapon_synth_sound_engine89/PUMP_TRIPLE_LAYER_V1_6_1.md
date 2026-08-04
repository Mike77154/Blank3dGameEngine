# Triple-layer shotgun pump — v1.6.1

## Signal roles

```text
gpump89       -> structural stage mass, rear stop, carrier, battery, lock
chuecka89     -> bright metal articulation and chic/chok definition
shotpumpkin89 -> continuous rail friction, microbursts and action-bar chatter
gklek89       -> midpoint carrier articulation
gweaponfoley89-> small trailing mechanical tik
```

## Synchronization

The shotpumpkin cycle is retimed from the selected gpump cycle:

- pull ends at approximately 40% (rear stop);
- reversal gap spans approximately 40-54%;
- forward pump ends at approximately 93% (lock).

The layer remains close and dry because the engine acoustic-world stack owns
room propagation. Defaults: `pump_shotpumpkin_q15 = 10800` and delay `8 ms`.

## Preview order

`wsse89_v1_6_1_pump_layers_ab.wav`:

1. gpump89 only;
2. chuecka89 only;
3. shotpumpkin89 only;
4. previous complete pump without shotpumpkin89;
5. new complete triple-layer pump.

`wsse89_v1_6_1_triple_pump_complete.wav` contains a normal service cycle and a
heavier cycle.
