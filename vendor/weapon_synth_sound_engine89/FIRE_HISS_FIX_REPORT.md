# Fire Hiss Fix Report — v1.3.0

## Problema

En el showcase anterior, dos ráfagas de lanzallamas se solapaban entre 7 y 10 s.
El preset tenía airflow y brightness demasiado altos, crackle bajo y una textura
broadband muy parecida entre ambas voces. El resultado era un `hiiis` continuo
parecido a ruido de televisión.

## Cambios de síntesis

- `airflow_q15`: reducido;
- `brightness_q15`: reducido;
- `pressure_q15`: elevado;
- `crackle_q15`: elevado;
- mayor peso de roar/body/jet en `gfire89`;
- hiss mezclado a 1/3 en lugar de 1/2;
- pulse depth y vortex lento para turbulencia modulada;
- parámetros correlacionados por semilla;
- segunda llama activa atenuada y oscurecida automáticamente;
- segunda ráfaga desplazada para evitar una superposición larga e idéntica.

## Medición 7.0–10.0 s

| Métrica | Resultado nuevo frente al viejo |
|---|---:|
| RMS total | 61.79 % (-38.21 %) |
| 20–250 Hz | 98.29 % (-1.71 %) |
| 4–16 kHz | 47.35 % (-52.65 %) |
| 8–18 kHz | 38.46 % (-61.54 %) |
| Pico viejo | 30,895 |
| Pico nuevo | 23,562 |
| Clipping | 0 |

La corrección quita principalmente energía aguda continua y conserva el cuerpo
grave. El cambio no es una simple bajada de volumen general.

## Archivos de escucha

- `audio/12_old_hiss_7_10_seconds.wav`
- `audio/13_corrected_flame_7_10_seconds.wav`
- `audio/14_AB_old_hiss_then_corrected_7_10.wav`
- `audio/15_AB_old_firefight_then_corrected_firefight.wav`
