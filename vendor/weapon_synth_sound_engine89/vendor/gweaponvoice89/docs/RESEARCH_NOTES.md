# Design references

- FMOD separates virtual channels from a smaller real-channel set chosen using priority and audibility: https://www.fmod.com/docs/2.03/api/core-api-system.html
- FMOD exposes channel audibility and documents priority as coarse voice-selection control: https://www.fmod.com/docs/2.03/api/core-api-channel.html
- Wwise combines playback limits, priority, distance offsets and virtual behavior: https://www.audiokinetic.com/en/public-library/2025.1.3_9039/?id=advanced_settings_tab_actor_mixer_objects&source=Help
- Wwise supports continue, kill, virtualize, resume and elapsed-time behavior: https://www.audiokinetic.com/en/public-library/2025.1.3_9039/?id=limiting_object_playback_instances&source=Help
- XAudio2 operation sets motivate the deferred batch API for synchronized starts: https://learn.microsoft.com/en-us/windows/win32/xaudio2/xaudio2-operation-sets

These were architecture references only. No third-party implementation code is included.
