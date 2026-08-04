# FPIL per-entity script routing fix (v3.9.2)

The GFO entity INI already carried `logic.script`, but the old object adapter ignored that path and ticked a single globally loaded FPIL context (`scripts/enemy.fpi`). Therefore every FPIL entity behaved like the zombie.

The language adapter now maintains a fixed static pool of FPIL contexts keyed by script path. `b3d_object_run_fpil()` calls `blank3d_languages_tick_fpil_path(path, entity)`, so each GFO executes the script selected by its own INI.

No dynamic allocation is used.
