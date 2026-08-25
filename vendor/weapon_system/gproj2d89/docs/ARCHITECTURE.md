# Arquitectura de GProj2D89

## Propiedad de memoria

```text
caller
  |-- gp2d_path paths[N]
  |-- gp2d_vec2 points[M]
  `-- gp2d_scene
         |-- path_count
         |-- point_count
         `-- error sticky
```

La biblioteca no retiene punteros fuera de `gp2d_scene`, no crea arenas internas y no mantiene estado global mutable.

## Capas

```text
[perfil de munición]
       |
       +--> [constructor de shell]
       |
       +--> [constructor de projectile]
                    |
                    +--> círculo / elipse / cápsula
                    +--> gota / round nose / spitzer
                    +--> polígonos auxiliares
                                |
                                v
                         paths etiquetados
```

## Roles semánticos

Los roles permiten que un renderer o editor cambie materiales sin entender la topología:

```text
SHELL_BODY / SHELL_RIM / PRIMER
PROJECTILE / JACKET / CORE / PELLET
BAND / FUZE / LEVER / PIN / FIN / NOZZLE / DETAIL
```

## Outline

El outline pertenece al path y se expresa en unidades de salida Q16.16. No se hornea en la silueta. Un backend puede usarlo como `stroke-width`, expandirlo a triángulos o ignorarlo.

## Rotación

La transformación usa una tabla de 64 pasos de seno/coseno Q16.16. No requiere biblioteca matemática. Para sprites de inventario, pickups y HUDs, 5.625 grados por paso suele ser suficiente y mantiene el runtime pequeño y determinista.
