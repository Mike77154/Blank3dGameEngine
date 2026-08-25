# mwinaudio89

Composition root Win32 con tres rutas independientes:

1. `mwinplaysound89`: WAV externo por `PlaySoundA(SND_FILENAME)`.
2. `mwavplayer89 + mwinmaudio89`: WAV decodificado a `waveOut`.
3. `mgeneralsynth89 + mgifwinaudio89`: Giffany UI General Synth89 v0.5 a `waveOut`.

No incrusta audio ni crea procesos, ventanas o archivos temporales.
