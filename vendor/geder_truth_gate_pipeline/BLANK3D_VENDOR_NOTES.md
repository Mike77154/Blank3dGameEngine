# Blank3D vendor notes

GEDER Truth Gate Pipeline 1.0.0 is vendored without changing its public API or
implementation. Blank3D compiles `src/geder_truth_gate.c` directly and places
all engine-specific policy in `src/blank3d_truth_gate.c`.

Standalone validation remains available with:

```sh
make -C vendor/geder_truth_gate_pipeline clean test
```
