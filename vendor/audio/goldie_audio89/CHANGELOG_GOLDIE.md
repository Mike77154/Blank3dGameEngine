# Goldie changelog

## 0.3.0
- Replaced logical recursive gain inheritance with true nested submix consoles.
- Every active console owns an independent rawmix engine and output PCM block.
- Child console output is injected as one protected channel into its parent.
- Added bottom-up render order, cycle protection and direct-child capacity checks.
- Added console-level gain/pan/mute, low-pass, drive, limiter, meter and stats APIs.
- Added console-aware voice handles and leaf playback API.
- Kept MASTER-only compatibility helpers for the v0.1 playback surface.
- Vendors remain unmodified.
