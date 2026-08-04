# Changelog

## 1.9.0 — Rocket propulsion and Gatling motor bed

- Vendored and linked `grocketspin89`, `ggunmach89`, `GGunTuberotator89` and `ggatlingwhistle89`.
- Added a stronger rocket launch stack: report, bipolar pressure, muzzle expansion and short launch plume.
- Added discreet in-flight `gfire89`, rocket spin and air-space whistle layers.
- Added a full Gatling timeline with simultaneous motor, barrel-rotator and whistle bed under an 80-shot burst.
- Added seven append-only runtime events for rocket spin and Gatling lifecycle control.
- Added integration tests, deterministic previews and sanitizer validation.

## v1.8.3 — Rocket Whistle v1.2

- Replaced the vendored `grocketwhistle89 v1.1` tree with v1.2.
- Added air-space delay, harmonic/roughness synthesis and post-reverb six-band EQ through the new vendor.
- Added `WSSE89_EVENT_ROCKET_WHISTLE_FX` and `wsse89_rocket_whistle_fx_defaults()`.
- Lowered the facade default whistle gain from 17500 to 15500 Q15.
- Updated full-capacity rocket timelines and added an event-facade stereo showcase.
- Added a vendor API regression test to the top-level Makefile.


## 1.8.1 — Magazine handling and rocket trajectory whistle

- Vendored and linked `wmagazine89 v1.0` and `grocketwhistle89 v1.1`.
- Added fixed caller-owned pools for four magazine voices and four whistle voices.
- Added append-only events for magazine action and whistle start/motion/release/stop.
- Updated pistol and sniper reload sequences with procedural magazine handling.
- Kept the machine-gun sequence belt-fed instead of forcing a box-magazine sound.
- Added sustained Doppler whistle between rocket ignition and impact in all rocket demos.
- Added strict-C89 integration test and updated game-engine adapter example.

## 1.8.0 — Full-capacity sequence showcase

- Added isolated-shot then full-capacity sequences for pistol (20), Magnum (6), shotgun (10), machine gun (40), sniper rifle (4), and rocket launcher (4).
- Added receiver resonance to the procedural report banks.
- Added synthesized magazine, loose-ammo, belt-feed, and launcher reload banks.
- Added a 106-second combined showcase.
- Fixed 32-bit overflow in long millisecond-to-frame timeline conversion.
- Added deterministic validation and zero-clipping metrics.

# Changelog

## 1.7.0 — Physical hierarchy and complete weapon sequences

- Added Realistic, Hybrid and Cinematic class-mix profiles to `gweaponvoice89`.
- Made reports and explosions dominate mechanisms while preserving readable close-detail Foley.
- Coupled `WSSE89_EVENT_WEAPON_MODE` to DNA, acoustic and voice-mix profiles.
- Rebalanced report, shotgun pump, grenade and rocket levels.
- Added complete procedural sequences for pistol, Magnum, shotgun, machine gun, sniper rifle, hand grenade, rocket launcher and grenade launcher.
- Added deterministic hierarchy tests, sanitizer coverage and WAV window validation.

## 1.6.2 — Shotgun pump master and physical model profiles

- Added `gshotguneq89`, a six-band fixed-point master dedicated to the pump mechanism bus.
- Added physical tuning profiles for Remington 870, Mossberg 500/590, Benelli Nova and Winchester SXP.
- Kept `gklek89` in the carrier/locking phase and retimed it per profile.
- Added optional external `tek` provider callbacks with the existing Foley particle as fallback.
- Added deterministic regression tests and three release previews.
- Bumped the unified umbrella version to 1.6.2.

# 1.6.1

- Vendored `shotpumpkin89` into the unified engine.
- Added it as a synchronized third layer in `gshotgunsequence89` pump cycles.
- Exposed shotpumpkin gain, delay and preset controls.
- Added isolated-layer, legacy-pair and triple-layer previews.
- Added deterministic triple-layer regression testing.

# Changelog

## 1.6.0 — Phase 3 Weapon DNA and validation

- Reworked `wsounddna89` around editable physical weapon descriptors.
- Added ten default weapon profiles and six mechanical action types.
- Added deterministic correlated energy, powder, mechanism and environment variation.
- Added coherent preset/gain application across the complete report stack.
- Added automatic pressure, receiver and muzzle-device coupling for profiled fire events.
- Added `wsoundmetrics89` fixed-point regression metrics and editable targets.
- Added five append-only events and public metrics/DNA queries.
- Added Phase 3 deterministic tests, profile preview and generated metrics report.

## 1.5.0 — Phase 1 + Phase 2 acoustic world

- Añadió `wsoundacoustic89`, C89/fixed-point y caller-owned.
- Añadió pulso de presión bipolar para reporte, impacto, granada y cohete.
- Añadió propagación low/mid/high con distancia, absorción atmosférica, directividad por fuente, oclusión, transmisión por material y portal.
- Añadió armónico de traducción grave para altavoces pequeños.
- Añadió bus dinámico multibanda, ducking de mundo y limitador.
- Añadió ocho reflexiones tempranas y cuatro líneas de reverberación tardía.
- Añadió once materiales y ocho espacios interiores/exteriores con ground bounce.
- Añadió perfiles REALISTIC, HYBRID y CINEMATIC.
- Añadió siete eventos acústicos al ABI sin retirar eventos anteriores.
- Añadió auto-pressure y auto-directivity para reportes, proyectiles, impactos, ricochets, granadas y cohetes.
- Añadió preview A/B y triptych de warehouse/urban/field.
- Conservó el stack world anterior como fallback.

## 1.4.3 — Long bank cursor overflow fix

- Corrige el reinicio periódico de bancos PCM de más de 65,535 frames en el demo realtime.
- Sustituye la posición Q16 empaquetada en `u32` por cursor entero de frame más acumulador fraccional.
- El rocket blast de cinco segundos ya no vuelve al principio cada 1.486 segundos.
- La corrección aplica también a reportes y colas largas de otras armas.
- No cambia el ABI público del motor ni el DSP de `wsound_rocketblast89`.

## 1.4.1 — Rocket rumble/crackle release

- Actualizó `wsound_rocketblast89` a v1.4.0.
- Amplió el banco del cohete de cinco a siete ruidos independientes.
- Añadió rumble subgrave y crackle irregular con decays separados.
- Añadió un SVF independiente para cada nueva capa.
- Añadió un LFO tímbrico dedicado al rumble; el LFO del chorus sigue siendo room-send-only.
- Expuso `wsrb89_params` completos dentro de `gsso89_rocket_params` para eventos de cualquier engine.
- Añadió helpers para cargar preset o activar parámetros personalizados.
- Añadió pruebas de determinismo y previews A/B/aislados.

## 1.4.0 — Unified event release

- Conectó todas las fuentes vendorizadas al build superior.
- Añadió `gsynthreport89`, `gsynthworld89` y la fachada `weapon_synth_sound_engine89`.
- Añadió 29 tipos de evento neutrales respecto del game engine.
- Añadió callback `wsse89_event_sink_fn` y adaptador de ejemplo.
- Añadió librería completa y librerías por capa.
- Endureció validaciones, conversiones temporales y memoria caller-owned.
- Añadió pruebas strict C89, ASan/UBSan, i386 y determinismo.
- Añadió preview unificado v1.4.

## v1.3.0

- Afinado `gfire89` a v1.2 para reducir el hiss de lanzallamas.
- Menos airflow y brightness; más pressure, cuerpo, turbulencia y crackle.
- Variación de timbre por semilla para evitar chorros idénticos.
- Ducking automático de la segunda voz de lanzallamas activa.
- Solapamiento del showcase reducido a una pequeña transición de release.
- Añadido `gsynthordnance89` como puente independiente y caller-owned.
- Integrado `gbulletair89 v1.2` mediante resampling Q16 48 kHz -> 44.1 kHz.
- Integrado `wsound_ggrenadeblast89 v1.0.2` como explosión crítica.
- Integrado `wsound_rocketblast89 v1.3.0` como explosión crítica.
- Añadidos demos aislados, escena conjunta y full-stack.
- Añadidos A/B de hiss viejo/nuevo y engine corregido/engine con ordnance.
- Cero clipping en todos los WAV de release.
- C89 estricto, protocolo sin heap y build i386 validados.

## v1.2.2

- `wsoundoutdoor89` convertido de insert maestro a envío wet-only.
- Fricción, térmicos, casquillos y partículas pequeñas dejan de alimentar el
  cañón urbano.

## v1.1

- Integrados `gtinkle89` y `gfire89` como proveedores directos del Voice Handler.
