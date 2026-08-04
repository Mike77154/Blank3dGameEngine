# Changelog

## 1.0.2

- Corregido el crackle residual en 7–8 s y 14–15 s del montaje.
- Reducida la densidad efectiva de fragmentos de cientos de retriggers por segundo a eventos discretos.
- Añadido bloqueo de retrigger mientras el pulso previo sigue activo.
- Añadido segundo low-pass fixed-point al ruido de fragmentos.
- Suavizada la envolvente de ataque de fragmentación.
- Añadida prueba `test_speaker_safe` para aspereza de alta frecuencia.
- Regenerados previews, montaje y comparación A/B de las ventanas exactas.


## 1.0.1

- Fixed hidden internal hard clipping before the six-band EQ.
- Smoothed and band-limited the fragment/crackle layer.
- Reduced pathological one-sample jumps while retaining a hard metallic crack.
- Regenerated all individual previews and the montage.

## 1.0.0

- Initial release.
