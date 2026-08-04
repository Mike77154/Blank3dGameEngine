# Secuencias mecánicas físicas v1.8.2

Este release corrige las líneas de tiempo del showcase sin cambiar el contrato de las librerías. La regla central es separar el **ciclo que ocurre por cada disparo** de la **manipulación que sólo ocurre al recargar**.

## Pistola semiautomática

Por tiro: report, recorrido corto de corredera, extracción/expulsión del casing, reset y retorno por resorte. El cargador sólo aparece al agotarse o durante una recarga explícita. El rack manual conserva un banco más largo y pesado que la corredera automática.

Referencia: Beretta APX/APX A1 official user manuals.

## Magnum / revólver

El perfil principal es double-action: el gatillo avanza el cilindro, arma y libera el martillo antes del report. Las vainas permanecen en el cilindro durante los seis tiros; apertura y extractor múltiple aparecen únicamente durante la recarga.

Referencia: Smith & Wesson revolver descriptions and owner-manual material.

## Escopeta pump-action

Después de cada tiro: rearward pump, extracción/expulsión del shell, carrier/elevator y forward pump para alimentar y cerrar. El ciclo conserva `shotpumpkin89 + chuecka89 + gpump89 + gklek89`, procesado por `gshotguneq89`.

Referencia: Mossberg 500/590 official owner manuals.

## Ametralladora belt-fed tipo M240

Cada tiro activa bolt/receiver, feed pawls, avance corto de cinta, casing y link en direcciones opuestas. La apertura de tapa, manipulación larga de cinta y carga completa sólo aparecen al comienzo o final de la recarga.

Referencia: US Marine Corps M240B Medium Machine Gun student handout.

## Rifle de francotirador bolt-action

Después de cada disparo: unlock, bolt rearward, extracción/expulsión, bolt forward, alimentación y lock. `wmagazine89` sólo se dispara en los bloques de recarga, nunca entre tiros normales.

Referencia: Winchester Model 70 official owner manual.

## Lanzacohetes

La secuencia conserva carga/pestillo, ignición, trayectoria y blast como sucesos separados. El silbido es una pista de movimiento y no el evento dominante: en el preview corto fue reducido 10.81 dB RMS respecto de v1.8.1; queda aproximadamente 15.5 dB RMS por debajo del impacto.

La intensidad exacta del silbido es una decisión de mezcla perceptual. El runtime conserva control de Doppler, distancia, paneo, release y stop para que el host lo conduzca mediante la trayectoria real.

## Bancos mecánicos añadidos al showcase

- `MECH_PISTOL_AUTO`: corredera automática corta con Foley y receiver; no sustituye el rack manual.
- `MECH_MACHINE_CYCLE`: chuecka/feed, Foley rápido, belt-feed y resonancia del receiver.
- Todos los bancos principales reciben `wsoundreceiver89` para evitar que los contactos parezcan muestras aisladas.

## Compatibilidad

- C89 estricto.
- Fixed-point / enteros.
- Sin heap.
- APIs runtime sin ruptura.
- Los cambios se concentran en la composición de secuencias y bancos del showcase.
