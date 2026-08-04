# Integration Report — v1.4.0

## Resultado

Todas las implementaciones runtime canónicas vendorizadas participan ahora en el grafo principal de compilación. Los writers WAV, demos, tests y copias duplicadas dentro de sub-vendors permanecen fuera de la biblioteca runtime. La biblioteca completa `libweapon_synth_sound_engine89.a` contiene síntesis base, reportes, ordnance, expansion pack, weapon world, voice handler y fachada de eventos.

## Capas conectadas

### 1. Reporte/disparo

- `gpaah89`
- `gweaponbody89`
- `gmuzzlegas89`
- `gballisticcrack89`
- `glatetail89`
- `gcinemathump89`
- bridge nuevo `gsynthreport89`

### 2. Mecánica y base

- `chuecka89`
- `gpump89`
- `gweaponfoley89`
- `gklek89`
- `gshotgunsequence89`
- `gtinkle89`
- `gfire89`
- `gfire89_fx`
- bridge `gsynthsoundengine89`

### 3. Ordnance

- `gbulletair89`
- `wsound_ggrenadeblast89`
- `wsound_rocketblast89`
- bridge `gsynthordnance89`

### 4. Expansion pack

- `wsoundmuzzledevice89`
- `wsoundaero89`
- `wsoundfriction89`
- `wsoundparticles89`
- `wsoundammo89`
- `wsoundbeltfeed89`
- `wsoundoutdoor89`
- `wsoundportal89`
- `wsounddoppler89`
- `wsoundspatial89`
- `wsoundthermal89`
- `wsoundlistener89`
- `wsoundmask89`
- bridge endurecido `gsynthsoundexpansion89`

### 5. Weapon world

- `wsounddna89`
- `wsoundreceiver89`
- `wsoundprojectile89`
- `wsoundprop89`
- `wsoundroom89`
- `wsoundaction89`
- `wsoundimpact89`
- `wsoundricochet89`
- `wsoundcombatbus89`
- bridge nuevo `gsynthworld89`

### 6. Gestión de voces

- `gvoice89`
- `gweaponvoice89`

### 7. Fachada pública

- `weapon_synth_sound_engine89.h/.c`
- `wsse89_event`
- `wsse89_dispatch`
- `wsse89_event_sink_fn`
- render PCM16 estéreo por muestra o por bloque

## Compatibilidad

Las cabeceras y bridges anteriores permanecen disponibles. El release añade una ruta unificada sin obligar a migrar código que ya utilice APIs directas.

## Endurecimiento aplicado

- validación de sample rate, presets y rangos Q15;
- conversión saturada de milisegundos a frames;
- comprobación completa de memoria caller-owned;
- propagación de errores de inicialización;
- corrección del sample rate al reiniciar voces de ricochet;
- pruebas de capacidad/rechazo de voz y dispatch;
- build del ejemplo de callback externo;
- preview generado exclusivamente mediante la API pública.

## Decisión de ABI

Los eventos comunes se normalizan en `wsse89_event`. Los secuenciadores mecánicos especializados se mantienen expuestos directamente para no perder parámetros ni imponer una representación pobre. Siguen conectados porque sus objetos forman parte de la biblioteca completa y sus cabeceras son reexportadas por el encabezado paraguas.
