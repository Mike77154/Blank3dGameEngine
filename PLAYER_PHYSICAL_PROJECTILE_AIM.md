# Player physical projectile camera aim — v3.26.2

## Problema

El lanzagranadas, el rocket launcher y la resortera seguían atravesando la ruta
antigua de zeroing de proyectiles. En TPS/OTS, un disparo sin impacto podía
reemplazar el objetivo real de la retícula por un plano de convergencia cercano
de 32 unidades. Al existir separación vertical y lateral entre cámara y muzzle,
ese target artificial producía una dirección visible inclinada o "chueca".

Las armas hitscan ya no sufrían esto porque desde v3.25.5 usan directamente el
raycast de la cámara. Los tres proyectiles físicos todavía dependían del target
generado por GWeapon89 y de una segunda reconciliación posterior.

## Ruta nueva, sólo para el jugador

`blank3d_player_projectile_aim_prepare()` reutiliza la autoridad de cámara ya
probada por `blank3d_player_fire_ray_resolve()`:

```text
cámara activa del frame
        |
        +--> raycast al centro de la retícula
                     |
                     +--> target real o camera + forward * range
                                      |
muzzle físico ------------------------+
                                      |
                                      +--> dirección muzzle -> target
                                                |
                       +------------------------+------------------+
                       |                        |                  |
                    rocket                  granada            resortera
                    linear                gravity arc            Bolt3D
```

- FPS, TPS centrada y OTS seleccionan el objetivo desde la cámara.
- TPS/OTS conservan la comprobación de obstrucción desde el muzzle.
- El origen físico permanece en el projectile/muzzle socket real.
- Rocket conserva dirección lineal.
- Granada y resortera reciben compensación de arco después de seleccionar el
  target, usando su gravedad y velocidad reales.
- No se dibuja el raycast autoritativo.
- NPCs no llaman esta ruta y conservan su pipeline previo.

## Archivos

- `src/blank3d_player_projectile_aim.c`
- `src/blank3d_player_projectile_aim.h`
- `tests/test_player_physical_projectile_aim.c`

## Regresión principal

La prueba coloca una cámara TPS/OTS elevada y detrás del muzzle. Verifica que el
target sin colisión permanezca a 120 unidades sobre el rayo central y no vuelva
a colapsar al plano antiguo de 32 unidades. También valida obstrucción desde el
muzzle y los backends de granada, cohete y Bolt3D.
