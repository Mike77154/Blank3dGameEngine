# Blank3D v3.24.0 release notes

- Added the GFO operation `draw_numbar("orchestrator.ini")`.
- Added a fixed-pool NumBar service with per-actor runtime instances.
- Split gameplay HUD responsibilities: ECG stays in `gameplay.bighud`; bars are
  instantiated by `gameplay.ini`.
- Added generic `numbar.*` binding channels to BigVaderHudder.
- Added natural preset canvases and independent X/Y instance scaling.
- Added exact anchor/X/Y/Z placement from INI.
- Added host bindings for player, self, target/enemy/boss and weapon values.
- Added 20 reusable `.bhud` presets covering the GBar89 v0.4 feature families.
- Added cache/lifetime handling: active instances keep visual state and stale
  actors release fixed slots.
- Added `test-numbar` and expanded GFO/BigHUD tests.
