# Design research notes

The architecture follows established game-audio patterns rather than mixing every logical channel unconditionally:

- FMOD documents a virtual-channel pool with a smaller real-channel subset selected by priority and audibility: https://www.fmod.com/docs/2.03/api/core-api-system.html
- FMOD exposes calculated audibility and states that lower-priority channels are stolen first: https://www.fmod.com/docs/2.03/api/core-api-channel.html
- Wwise documents playback limits, priorities, distance priority offsets and virtual behaviors such as continue, resume, play from elapsed time and kill: https://www.audiokinetic.com/en/public-library/2025.1.3_9039/?id=advanced_settings_tab_actor_mixer_objects&source=Help
- Wwise warns about virtual voices that never return or end, motivating `virtual_timeout_ms`: https://www.audiokinetic.com/en/public-library/2025.1.4_9062/?id=ErrorCode_VirtualVoiceLimit&source=Help
- XAudio2 operation sets apply grouped voice changes atomically, motivating the batch API: https://learn.microsoft.com/en-us/windows/win32/xaudio2/xaudio2-operation-sets

No source code from those systems is included.
