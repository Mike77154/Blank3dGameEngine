# Blank3D v3.27.7 — Audio + text provider foundation

- Vendored GoldieAudio89 under `vendor/audio/goldie_audio89`.
- Weapon synthesis now enters Goldie's `SFX/Weapons` submix through a generic PCM callback.
- Goldie is the authoritative mixer; the existing WinMM host remains the final device queue.
- Added engine-facing bus gain/mute/PCM playback plus WAV and Win32 MP3 decode wrappers.
- Vendored Monika FontCore under `vendor/text/monika_fontcore`.
- Added engine-level `Blank3DText89`; BVHUD consumes it as a provider rather than owning it.
- HUD counters use FontCore UTF-8 metrics + antialiased TTF/glyf rasterization when a project font is configured.
- Existing 5x7 text remains a safe fallback.
- No font asset is bundled; `text.font` is project supplied.
