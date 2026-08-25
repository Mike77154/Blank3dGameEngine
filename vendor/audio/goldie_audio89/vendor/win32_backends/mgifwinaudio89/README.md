# mgifwinaudio89

WinMM streaming backend for `mgeneralsynth89`. It keeps four fixed stereo
buffers and is pumped from the host timer; no thread, heap, temporary WAV, or
embedded PCM is used. The original eight-voice synth player remains active.
