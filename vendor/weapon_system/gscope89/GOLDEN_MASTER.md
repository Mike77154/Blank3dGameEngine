# Golden master validation v0.4

The pre-INI 192-reticle implementation is frozen under `tests/golden/legacy/`.
The v0.4 recipe implementation is compared against it with the same fixed-point
C89 preview raster backend.

`make golden_master` builds two independent executables:

1. legacy hardcoded preset catalog -> command stream + 320x320 PPM;
2. INI catalog -> command stream + 320x320 PPM.

For every preset the test compares:

- serialized draw-command bytes;
- final PPM bytes.

Result:

```text
golden_presets=192
visual_byte_failures=0
command_byte_failures=0
```

Current ordered corpus hashes:

```text
PPM SHA-256 legacy = a49d90b6279441c97fd3a31c8c5935e9131833626e31e0277600a2ac9ff00b95
PPM SHA-256 INI    = a49d90b6279441c97fd3a31c8c5935e9131833626e31e0277600a2ac9ff00b95
CMD SHA-256 legacy = 230f2a698976ca2f4ae6c83445d795c4d14e220859ff01ac4b1d7f4413496cb7
CMD SHA-256 INI    = 230f2a698976ca2f4ae6c83445d795c4d14e220859ff01ac4b1d7f4413496cb7
```

The default animation set is deliberately event-driven only, so an idle pose is
identity and does not alter the golden visual. Optional always-on movement lives
in the separate `reticle_lively` animation set.
