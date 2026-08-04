# GFO build fix v3.8.1

- Removed the obsolete `run_ddsl2_file()` and `run_fpi_for_enemy()` wrappers left behind after moving logic dispatch into one GFO per entity.
- `b3d_object_run_ddsl2()` now calls the vendored DDSL2 runtime adapter directly, avoiding the implicit/non-static declaration conflict.
- Removed the unused `clamp_fix()` helper.
- Replaced warning-prone bounded `strncpy` calls in the weapon INI adapter and DDSL2 store with explicit C89 bounded-copy helpers.
- Verified with `make syntax-check` and `make test-gfo-entities`.
