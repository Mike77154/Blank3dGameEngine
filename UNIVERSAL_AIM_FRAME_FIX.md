# Blank3D v3.25.3 — Universal Aim Frame

## Síntoma confirmado

La captura de ejecución mostró la cruz del HUD en el centro mientras el arma y
los trazadores salían hacia arriba/derecha. Al avanzar también podían aparecer
proyectiles orientados hacia atrás. El comportamiento se repetía con varias
armas, por lo que no era una falla exclusiva de la Gatling ni de un perfil INI.

## Causa raíz

El player tenía dos marcos de referencia válidos, pero incompatibles:

1. `player.weapon -> GAttach -> NationalMecanicanimal89` heredaba la base del
   cuerpo/socket del actor.
2. El HUD, el raycast de la cruz y el zeroing usaban la base de Cameranaku.
3. Los proyectiles lineales ya alineados al HUD podían pasar después por una
   segunda resolución genérica de lanzamiento.
4. No existía una compuerta final común que rechazara velocidades dentro del
   hemisferio trasero de la cámara.

Al moverse o cambiar pitch/yaw, el "adelante" del arma y el "adelante" visible
podían divergir. Como todas las armas del player usan el mismo carrier, el error
se propagaba a todo el catálogo.

## Corrección común

Se añadió `blank3d_universal_aim`, una capa C89 sin estado dinámico que construye
un solo frame inmutable por actualización/disparo:

```text
Cameranaku: eye + forward + right + up
                    |
                    v
          target del centro del HUD
                    |
          +---------+----------+
          |                    |
          v                    v
  carrier visual          evento balístico
  arma local -Z           snapshot de cámara
          |                    |
          +---------+----------+
                    v
       linear: conservar dirección HUD
       gravity/Bolt3D: resolver caída una vez
                    |
                    v
       compuerta de hemisferio delantero
                    |
                    v
        velocidad + trazador + proyectil
```

### Reglas resultantes

- La posición del socket sigue perteneciendo al actor; sólo se sustituye su base
  por la dirección `carrier -> target HUD`.
- Pistol, machine gun, shotgun, magnum, sniper y Gatling conservan directamente
  la dirección ya convergida al HUD.
- Grenade launcher y slingshot/Bolt3D reciben su resolución física una sola
  vez y después pasan por la misma compuerta frontal; rocket launcher conserva
  el backend lineal del catálogo.
- Un hit detrás del plano del carrier se descarta y se reconstruye sobre un
  plano de zeroing estable a 32 unidades.
- Una dirección final con `dot(direction, camera_forward) <= 0` se repara antes
  de crear velocidad, trail o cuerpo Bolt3D.
- La separación izquierda/derecha de la escopeta se conserva.

## Archivos principales

- `src/blank3d_universal_aim.h`
- `src/blank3d_universal_aim.c`
- `src/monika_blank3d.c`
- `tests/test_universal_weapon_aim.c`
- `Makefile`

## Regresión específica

`make test-universal-weapon-aim` recorre:

- cinco posiciones distintas del carrier para simular desplazamiento;
- las nueve armas del catálogo;
- un evento Gatling deliberadamente invertido;
- spread izquierdo y derecho de escopeta;
- backends lineal, gravedad y Bolt3D.

La prueba exige que presentación y lanzamiento permanezcan en el hemisferio
visible y que las armas lineales no pierdan la dirección convergida al HUD.

## Nota sobre v3.25.2

La v3.25.2 corrigió el snapshot y el retraso de sockets, pero fue insuficiente:
no unificó la base visual del carrier con Cameranaku ni eliminó la segunda
resolución común. La v3.25.3 sustituye ese parche parcial por una corrección en
la ruta compartida de todas las armas.
