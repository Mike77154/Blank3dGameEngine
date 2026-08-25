# Catálogo modular de cámaras por INI — v3.26.0

Blank3D ya no contiene un selector rígido `TPS / OTS / FPS`. Cameranaku carga
un catálogo de perfiles desde un directorio y considera **cada archivo `.ini`
como una cámara independiente**.

```text
config/
└─ cameras/
   ├─ tps_centred.ini
   ├─ ots.ini
   ├─ fps.ini
   └─ camera_profile.ini.example   # plantilla; no se carga
```

El catálogo usa almacenamiento fijo, sin `malloc`, `realloc`, `free`, heap,
`float` ni `double` en la integración. El límite actual es de 16 cámaras y se
puede ajustar con `B3D_CAMERA_PROFILE_MAX`.

## Selección de carpeta y cámara inicial

`config/blank3d.toml` sólo conserva la configuración global del catálogo:

```toml
[camera]
profile_dir = "config/cameras"
start_profile = "tps_centred"
lock_mouse = true
zoom_fov = 45.0
sniper_zoom_fov = 18.0
zoom_speed = 120.0
```

- `profile_dir`: carpeta examinada al iniciar el programa.
- `start_profile`: valor de `profile.id` que se activa al arrancar.
- Si la carpeta falta o no contiene perfiles válidos, se usan fallbacks
  internos de TPS centrada, OTS y FPS para no dejar al runner sin cámara.

## Añadir una cámara sin recompilar

1. Copiar `camera_profile.ini.example` dentro de `config/cameras/`.
2. Cambiar la extensión final a `.ini`.
3. Asignar un `profile.id` único.
4. Ajustar `profile.order` para decidir su posición en el ciclo.
5. Reiniciar el runner.

Ejemplo:

```ini
[profile]
id=isometric
name=Isometric Camera
order=40
enabled=true
rig=orbit
view_style=third_person
aim=camera_muzzle

[render]
player_body=true
player_weapon=true

[rig]
pivot_x=0.0
pivot_y=7.0
pivot_z=0.0
look_x=0.0
look_y=0.0
look_z=0.0
distance=12.0
min_distance=1.0
max_distance=32.0
offset_right=0.0
offset_up=0.0
offset_forward=0.0

[look]
pitch_min=-45.0
pitch_max=45.0
mouse_sensitivity=0.12

[lens]
fov=55.0
near=0.1
far=200.0

[smoothing]
position=0.40
rotation=0.50
fov=0.50

[collision]
enabled=true
radius=0.30
```

No existe una lista de nombres compilada. El archivo nuevo aparece en el
catálogo por tener extensión `.ini` y `enabled=true`.

## Controles y scripting

- `V`: avanza al siguiente perfil habilitado, ordenado por `profile.order` y
  luego por `profile.id`.
- `camera_profile <id>`: selecciona un perfil por identificador.
- `camera <id>`: alias corto de un argumento.

El cambio se refleja en el nombre mostrado por la ventana. El catálogo se
carga al inicio; esta versión no hace hot reload mientras el runner está
abierto.

## Referencia de campos

### `[profile]`

| Campo | Uso |
|---|---|
| `id` | Identificador estable para TOML y scripts. |
| `name` | Nombre visible. |
| `order` | Orden del ciclo. |
| `enabled` | Incluye o excluye el archivo del catálogo. |
| `rig` | `fps` u `orbit`. |
| `view_style` | `fps`, `third_person` u `over_shoulder`. |
| `aim` | `camera` o `camera_muzzle`. |

`rig` define cómo se construye físicamente la cámara. `view_style` informa al
weapon system cómo debe presentar el arma. `aim` decide si el raycast del
jugador depende sólo de la cámara o si usa la comprobación de dos rayos
cámara→objetivo y muzzle→objetivo.

### `[render]`

- `player_body`: dibuja u oculta el cuerpo del jugador.
- `player_weapon`: dibuja u oculta el arma montada.

### `[rig]`

- `pivot_x/y/z`: pivote relativo al actor.
- `look_x/y/z`: corrección del punto observado.
- `distance`: distancia orbital.
- `min_distance`, `max_distance`: límites de distancia.
- `offset_right`: desplazamiento de hombro; `0` mantiene TPS centrada.
- `offset_up`, `offset_forward`: desplazamientos finales del rig.

### `[look]`, `[lens]` y `[smoothing]`

- Límites verticales, sensibilidad, FOV, planos near/far y factores de
  seguimiento por perfil.
- Todos se leen con `conf_total` y se convierten a fixed-point Q20.12.

### `[collision]`

El perfil conserva `enabled` y `radius` para la integración del probe de
colisión de Cameranaku. El radio ya viaja con el perfil; el resultado efectivo
depende de que el runner suministre un `world_probe` en la escena activa.

## Perfiles incluidos

### `tps_centred.ini`

Vista orbitante centrada, más alta y distante. Ve al personaje ligeramente
desde arriba y no usa offset lateral.

### `ots.ini`

Vista cercana sobre el hombro, con FOV menor y offset lateral independiente.
No comparte sus números con la TPS centrada.

### `fps.ini`

La cámara nace en el pivote de ojos, oculta cuerpo/arma de mundo y usa el rayo
de cámara como autoridad de impacto.

## Aislamiento de NPC

El catálogo sólo gobierna la cámara y el `view_style` del jugador. Los NPC no
leen estos perfiles, no cambian de cámara y conservan su pipeline de muzzle,
IA y weapon manager.

## Pruebas

```text
make test-camera-profile-catalog
make test-cameranaku-provider
make test-automatic-fire-frame-lock
make test-player-camera-fire-ray
make syntax-check
make audit
```

`test-camera-profile-catalog` agrega una cuarta cámara de prueba únicamente
mediante un INI y confirma que aparece en el catálogo y en el orden esperado.
