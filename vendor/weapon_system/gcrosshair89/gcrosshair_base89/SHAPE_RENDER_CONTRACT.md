# Contrato de render de figuras ABI 2

`gcrosshair_params89` solo resuelve parametros. El renderer consume
`GC89_DrawSpec` y decide como rasterizar lineas, arcos, texturas o ambas.

## Reglas generales

1. El centro final es el centro de pantalla mas `center_offset_x_fx` y
   `center_offset_y_fx`.
2. La animacion de escala del core se aplica despues de resolver el spread.
3. `thickness_fx`, `dot_enabled`, `dot_size_fx`, `color_rgba`, imagen y tinte
   funcionan igual para todas las familias.
4. `shape_rotation_deg_fx` gira la geometria alrededor del centro.
5. Un bit apagado en `shape_segment_mask` omite esa pieza.
6. Los radios son semiejes, no diametros.
7. `shape_break_fx` abre un corte centrado en cada lado, arco o pieza activa.
   Cero significa trazo continuo. El renderer debe limitar el corte para que
   nunca invierta los extremos del segmento.

## GC89_SHAPE_CROSS

Usa el contrato original:

- `gap_fx`: separacion entre centro e inicio de cada brazo;
- `arm_length_fx`: largo del brazo;
- `arm_mask`: izquierda, derecha, arriba y abajo.

## GC89_SHAPE_CIRCLE

Dibuja una elipse con radios `shape_radius_x_fx` y `shape_radius_y_fx`.
La mascara representa ocho octantes, comenzando arriba y avanzando en sentido
horario:

```text
             7 | 0
          6    |    1
        -------+-------
          5    |    2
             4 | 3
```

Con `GC89_SEGMENT_ALL` se obtiene el contorno completo.

## GC89_SHAPE_SQUARE

Vertices del rectangulo envolvente y lados:

```text
       segment 0
    +-------------+
  3 |             | 1
    +-------------+
       segment 2
```

Los semiejes permiten tambien rectangulos aunque el preset se llame cuadrado.

## GC89_SHAPE_DIAMOND

Vertices: arriba, derecha, abajo, izquierda. Los segmentos avanzan en sentido
horario:

```text
          top
        /0   3\
 right <       > left
        \1   2/
         bottom
```

## GC89_SHAPE_CHEVRONS

`shape_direction_mask` activa uno o varios chevrons:

- `UP`: `^`;
- `DOWN`: `v`;
- `LEFT`: `<`;
- `RIGHT`: `>`.

Los radios definen el limite exterior y `shape_depth_fx` la distancia entre
la punta y la base de las dos alas. Para un chevron doble o compuesto basta
activar varias direcciones.

## GC89_SHAPE_HEXAGON

Seis vertices alrededor de los semiejes. El vertice cero esta arriba y los
demas avanzan en sentido horario. `shape_segment_mask` usa los bits 0 a 5;
cada bit conecta el vertice del mismo numero con el siguiente.

## GC89_SHAPE_BRACKETS

`shape_direction_mask` activa los brackets izquierdo, derecho, superior o
inferior. Cada bracket tiene una barra principal paralela al borde y dos
ganchos hacia el centro. `shape_depth_fx` es el largo de esos ganchos.

Ejemplo izquierdo y derecho:

```text
[             ]
```

## GC89_SHAPE_OPEN_TRIANGLE

Vertices base antes de la rotacion:

- vertice 0: arriba;
- vertice 1: abajo-derecha;
- vertice 2: abajo-izquierda.

Segmentos:

- bit 0: vertice 0 a vertice 1;
- bit 1: vertice 1 a vertice 2;
- bit 2: vertice 2 a vertice 0.

Por eso un triangulo sin base usa:

```c
GC89_SEGMENT_0 | GC89_SEGMENT_2
```

La rotacion permite apuntarlo arriba, abajo o hacia los lados sin cambiar la
mascara. Para un triangulo de tres esquinas separadas, active los tres lados y
use `gcp89_variant_set_shape_break_px` para cortar el centro de cada uno.

## Imagen e hibrido

`GC89_DRAW_IMAGE` ignora la geometria vectorial. `GC89_DRAW_HYBRID` dibuja la
figura y la imagen con el orden de capas definido por el core. Ningun campo de
imagen fue eliminado ni reinterpretado.
