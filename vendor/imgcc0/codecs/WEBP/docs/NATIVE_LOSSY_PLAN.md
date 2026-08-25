# Native lossy plan

API pública nueva:

- `GWPEstimateVP8LossyPlanScratch()`
- `GWPAnalyzeVP8LossyPlan()`
- `GWPVP8LossyPlan`
- `GWPVP8LossyMacroblockStat`

CLI:

```bash
./examples/gwpvp8plan --quality 72 --preset photo --dump-mbs in.pam
```

El objetivo es entregar una vista estable, sin `malloc`, del análisis previo al encode lossy:

- macroblock grid
- luma media y varianza por MB
- MB con alpha
- `segment_id` sugerido
- `filter_strength` sugerido por MB
- sugerencias globales para `segments`, `partitions` y `sharpness`
