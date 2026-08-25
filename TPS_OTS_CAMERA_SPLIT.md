# Blank3D v3.25.5 — Separación TPS / OTS / FPS

## El problema anterior

La cámara llamada TPS aplicaba siempre un desplazamiento lateral de hombro.
Eso mezclaba dos presentaciones distintas:

- una cámara de tercera persona centrada y orbital;
- una cámara over-the-shoulder orientada al combate.

Al compartir el mismo modo, era difícil razonar sobre retícula, parallax,
obstrucciones y origen visual del disparo.

## Modos nuevos

### TPS centrada

```text
          cámara
             |
             v
          [actor]
             |
          retícula
```

- Órbita detrás del actor.
- Sin desplazamiento lateral de hombro.
- Rayo de selección desde el centro de la cámara.
- Segundo rayo desde el muzzle para impedir disparar a través de cobertura.

### OTS clásica

```text
      cámara
          \
           \  retícula
        [actor]------X
           \
            muzzle
```

- Usa la misma órbita estable de TPS.
- Aplica el desplazamiento lateral sólo después de resolver la pose orbital.
- Mantiene la retícula como autoridad.
- Resuelve parallax con el segundo rayo muzzle -> aim_point.
- Está pensada como base para una presentación tipo RE4/RE5, sin afirmar que
  ya replica todos sus valores de FOV, distancia, hombro o animación.

### FPS

- Cámara a la altura de ojos.
- Cuerpo y arma de tercera persona ocultos.
- Un solo raycast desde la cámara hacia el centro del viewport.
- El muzzle visual no participa en el impacto.

## Control

La tecla `V` recorre:

```text
TPS centrada -> OTS -> FPS -> TPS centrada
```

## CameraNaku

Se añadieron tres modos explícitos:

```c
B3D_CNK_CAMERA_FPS
B3D_CNK_CAMERA_TPS
B3D_CNK_CAMERA_OTS
```

El proveedor publica estilos distintos al weapon manager:

```text
FPS -> GWP89_VIEW_FPS
TPS -> GWP89_VIEW_THIRD_PERSON
OTS -> GWP89_VIEW_OVER_SHOULDER
```

El offset `shoulder` sólo se aplica en OTS.

## Próximo ajuste visual recomendado

Después de confirmar que la balística ya no se despega de la retícula, la OTS
puede calibrarse sin tocar el disparo:

- distancia de cámara;
- altura del pivot;
- offset lateral;
- FOV normal y al apuntar;
- cambio de hombro;
- colisión/retracción de cámara;
- posición del personaje en pantalla;
- sensibilidad separada al apuntar.
