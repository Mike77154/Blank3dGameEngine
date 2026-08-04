# Dive enemy / picoteo aéreo — v3.10.3

## Objetivo

`dive_enemy` es una prueba simple de enemigo volador que:

1. persigue al jugador únicamente en X/Z;
2. intenta conservar una banda de vuelo de 7 a 9 unidades sobre el jugador;
3. cuando queda prácticamente sobre su eje vertical, guarda su posición aérea;
4. desciende directamente hacia el centro del jugador;
5. al impactar causa daño;
6. regresa a la posición guardada;
7. repite el ciclo.

El enemigo aparece desde `scripts/startup.rpy`:

```text
    dive_enemy 4 pos 0 8 12 hp 30
```

También se acepta el alias RPYL `dive_bomber_enemy`.

## Máquina de estados

```text
STATE 0: HOVER / ALIGN
  - mantener Y entre player.y + 7 y player.y + 9
  - acercarse en X/Z con moveforeflat
  - al quedar a <= 1.5 en X/Z, guardar la percha

STATE 1: DIVE
  - diveplayer=14 hacia el centro del jugador
  - al quedar a <= 1 unidad, hurtplayer=8

STATE 2: RETURN
  - lookatreturnposition
  - returntoposition=12
  - al quedar a <= 0.25 de la percha, volver a STATE 0
```

## Nuevas condiciones FPIL

```text
belowplayerheight=N
belowplayeroffset=N
aboveplayerheight=N
aboveplayeroffset=N
heightaboveplayeratleast=N
playerbelowby=N
returnpositionwithin=N
homewithin=N
returnpositionfurther=N
homefurther=N
```

`belowplayerheight=N` significa que el enemigo está por debajo de
`player.y + N`; `aboveplayerheight=N` significa que está por encima de ese
mismo objetivo.

## Nuevas acciones FPIL

```text
saveposition
savereturnposition
savehome

diveplayer=N
ramplayer=N
movetowardplayer=N

returntoposition=N
movetohome=N
returnhome=N

lookatreturnposition
lookathome
```

`diveplayer` y `returntoposition` usan avance fixed-point clampado: nunca
recorren más distancia que la restante. Esto evita atravesar el objetivo y
oscilar alrededor del jugador o de la percha.

## Tuneo rápido

Todos los valores se encuentran en `scripts/dive_enemy.fpi`.

```text
altura mínima sobre jugador = 7
altura máxima sobre jugador = 9
alineación X/Z              = 1.5
seguimiento horizontal      = 5
ajuste vertical             = 4
picada                      = 14
regreso                     = 12
daño                        = 8
```

Esta versión no añade animación, gravedad ni trayectoria curva. La picada y el
regreso son desplazamientos directos para comprobar primero el patrón de IA.
