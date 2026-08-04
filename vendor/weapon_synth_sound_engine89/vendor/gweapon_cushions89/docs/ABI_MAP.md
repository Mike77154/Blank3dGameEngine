# Suggested host ABI map

```text
weapon.fire event
  |-- base report provider (gpaah89 or custom)
  |-- body processor       (gweaponbody89)
  |-- gas generator        (gmuzzlegas89)
  |-- ballistic event      (gballisticcrack89)
  |-- late environment FX  (glatetail89)
  `-- cinema sweetener     (gcinemathump89)
```

The host owns contexts and chooses whether to trigger each layer. Geometry, distance, occlusion, inventory, camera, and physics remain outside this bundle.
