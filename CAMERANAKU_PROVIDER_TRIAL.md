# CamaraNaku89 v3.4 Receive-Provider Trial — Blank3D v3.6

## Objetivo

CamaraNaku89 pasa a ser la autoridad de vista del jugador sin apropiarse del
backend matemático ni del weapon manager. La integración forma una cadena de
doble provider:

```text
mouse / V / Ctrl / sniper sway / weapon recoil
                    |
                    v
             CamaraNaku89
        pose FPS/TPS + basis + FOV
          |                    |
          | receive-provider   | GWP89_SERVICE_CAMERA
          v                    v
 Gamlib3D move/scale/rotate   gweapon89 manager
          |                    |
          +---------> OpenGL <-+
```

## Receive-provider de transform

CamaraNaku se configura en `CNK_TRANSFORM_MODE_RECEIVE_PROVIDER`. Sus callbacks
son implementados por `src/blank3d_cameranaku.c` y delegan en Gamlib3D:

- `move`: suma vectorial Gamlib3D.
- `scale`: multiplicación fixed-point Q20.12.
- `rotate`: base local obtenida mediante `Transform` y ejes Gamlib3D.

La frontera corrige una diferencia de convención importante:

- CamaraNaku considera `+Z` como frente canónico.
- Gamlib3D expone `-Z` como frente local.
- El adaptador invierte únicamente la pata Z de la base y traduce el yaw
  almacenado de Gamlib (`180°` para mirar a `+Z`) al yaw lógico de CamaraNaku
  (`0°` para mirar a `+Z`).

## Provider hacia el weapon system

Se registra un provider de prioridad 140:

```c
gwp89_add_provider(&g.weapon_manager,
                   GWP89_SERVICE_CAMERA,
                   140,
                   "cameranaku89.receive-provider",
                   &g.cameranaku,
                   blank3d_cameranaku_weapon_provider);
```

Atiende `GWP89_OP_GET_CAMERA` y entrega:

- `camera_id = 34`
- estilo FPS o over-shoulder
- origen efectivo
- forward/right/up efectivos
- zoom fixed-point Q16.16

Los sockets de muzzle/aim y la dirección inicial de los proyectiles se forman
con esos mismos ejes. El sway del sniper modifica la base efectiva antes de que
la consulte el arma.

## Recoil y shake

`FIRE_ACCEPTED` ya no altera una cámara paralela. El recoil entra a CamaraNaku:

- suma pitch real sujeto a límites;
- agrega shake local mediante `cnk_camera_add_shake_ex`;
- el siguiente frame exporta esa pose al weapon provider.

Esto mantiene alineados retículo, muzzle, trayectoria y respuesta visual.

## Responsabilidades en esta prueba

| Responsabilidad | Autoridad |
|---|---|
| yaw, pitch, FPS/TPS, pose, smoothing | CamaraNaku89 |
| zoom/FOV solicitado por sniper | CamaraNaku89 + sniper stack |
| recoil/shake de arma | CamaraNaku89 |
| move/scale/rotate matemáticos | Gamlib3D receive-provider |
| consulta de cámara de armas | GWP89 camera provider |
| proyección y dibujo | backend Gamlib/OpenGL existente |
| colisión de proyectiles | CCS + SICOL-DE |
| spring-arm camera collision | no conectada todavía en v3.6 |

La colisión de cámara se deja deliberadamente fuera de esta primera prueba para
verificar primero que no haya doble autoridad ni discrepancias de ejes.

## Validación

`make test-cameranaku-provider` verifica:

1. llamadas reales a move/scale/rotate;
2. conversión de yaw Gamlib 180° a frente mundial +Z;
3. exportación correcta de estilos FPS/TPS;
4. atención de `GWP89_OP_GET_CAMERA`;
5. llegada de recoil y shake a la cámara activa;
6. conversiones Q20.12, Q24.8 y Q16.16 en los límites ABI.
