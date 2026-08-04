# Notas físicas y traducción al sintetizador

## 1. Frente de presión

Una explosión atmosférica produce un frente de choque con aumento de presión
casi discontinuo. Después aparece una fase positiva que decae hacia la presión
ambiente y, por sobreexpansión del aire, una fase negativa por debajo de la
presión ambiente.

### Traducción DSP

```text
muestra 0        : salto positivo
primeros ms      : caída exponencial rápida
después          : lóbulo negativo más largo y más débil
cola             : ruido, osciladores graves y reflexiones
```

El motor usa una aproximación bipolar barata. No calcula sobrepresión real,
masa de carga, distancia de seguridad ni daño.

## 2. Espectro

El frente corto aporta banda ancha. La parte grave proviene del contenido
de baja frecuencia del pulso, de su duración y de la interacción con suelo,
estructuras y aire. Un trabajo acústico analítico reciente obtiene forma de
onda y espectro en tercios de octava a partir de una forma Friedlander y
considera el ensanchamiento no lineal del pulso y su corrimiento de frecuencia.

### Traducción DSP

- Noise 0: masa central.
- Noise 1: energía grave y medio-grave.
- Noise 2: borde rápido del frente.
- Sine y Saw: retumbar audible en altavoces comunes.
- preset distante: menos agudos y duraciones mayores.

## 3. Fase negativa

La fase negativa no es un segundo estallido. Es una depresión más larga y de
menor amplitud. En v1.0.0 se dispara una sola vez después de la fase positiva;
una prueba de regresión evita que vuelva a arrancar periódicamente.

## 4. Fragmentos y superficies

Una munición de fragmentación añade multitud de sucesos secundarios. No es
correcto representar cada fragmento como tono: se usa ruido agudo con
microenvolventes aleatorias. Las superficies cambian esa capa:

- concreto: más eventos brillantes y escombro;
- tierra: absorción de agudos y cola grave;
- recinto: reflexiones más densas;
- aire libre: reverb mínima.

## 5. Distorsión, chorus y reverb

La distorsión no representa una propiedad física directa de la explosión.
Representa saturación de captación, altavoz y mezcla de videojuego.

El chorus es deliberadamente corto, lento y de mezcla baja. Un chorus profundo
convierte el grave en un barrido periódico perceptible, el indeseado
“wiu-wiu”.

La reverb se usa como aproximación de reflexiones. Los presets abiertos tienen
envío muy bajo; el preset confinado aumenta mezcla y realimentación.

## Referencias

1. Sam E. Rigby, Andrew Tyas, Terry Bennett, Sam D. Clarke y Stephen D. Fay,
   “The Negative Phase of the Blast Load”, International Journal of Protective
   Structures, vol. 5, núm. 1, 2014.
2. E. M. Salomons, “Analytical model for sound of explosives and firearms”,
   Journal of the Acoustical Society of America, 156(3), 2034–2044, 2024.
   DOI: 10.1121/10.0030301.
3. NIOSH, “Measurement of Exposure to Impulsive Noise at Indoor and Outdoor
   Firing Ranges during Tactical Training Exercises”, HETA 2013-0124-3208.
4. G. R. Price y colaboradores, “Weapon noise exposure of the human ear
   analyzed with the AHAAH model”, U.S. Army Research Laboratory.
5. “Mechanisms and Treatment of Blast Induced Hearing Loss”, Korean Journal
   of Audiology, 2013; material de revisión sobre historial presión-tiempo.
6. W. A. Ahroon, R. P. Hamernik y S. F. Lei, “The effects of reverberant blast
   waves on the auditory system”, JASA 100(4), 1996.
