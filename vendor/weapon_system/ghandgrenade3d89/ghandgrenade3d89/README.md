# ghandgrenade3d89

Biblioteca C89 de **mallas 3D low-poly de granadas de mano**, diseñada como catálogo visual y generador determinista.

## Restricciones del núcleo

- C89 estricto.
- Sin `malloc`, `realloc`, `free` ni heap.
- Sin `float` ni `double` dentro de la biblioteca.
- Coordenadas fixed-point Q24.8 en milímetros.
- Buffers entregados por el usuario.
- Tres partes independientes por modelo: `BODY`, `HOLDER` y `SAFETY`.
- Pivotes por parte para animación externa.
- 41 presets de siluetas.
- Tres LODs.

> El exportador OBJ de herramientas convierte a texto decimal usando `double` únicamente al imprimir el archivo. Ese código no forma parte del núcleo ni del ABI de runtime.

## Uso mínimo

```c
GHG3D_Desc desc;
GHG3D_Result result;
static GHG3D_Vertex vertices[4096];
static GHG3D_Triangle triangles[8192];

ghg3d_desc_default(&desc, GHG3D_PRESET_PINEAPPLE_CLASSIC);
ghg3d_build(&desc, vertices, 4096, triangles, 8192, &result);
```

## Partes animables

- `GHG3D_PART_BODY`: carcasa principal; en variantes de mango incluye cuerpo y mango.
- `GHG3D_PART_HOLDER`: palanca, sujetador, tapa o collar superior.
- `GHG3D_PART_SAFETY`: pasador y anillo; en presets históricos sin palanca puede ser anillo inferior o lazo.

La librería no incluye temporizadores, detonación, físicas, daño, explosivos ni lógica de uso. Sólo construye geometría visual.

## Compilar

```sh
make
./test_all_presets
mkdir -p generated
./export_all_obj generated
```

MinGW32:

```bat
build_mingw32.bat
```

## Licencia

CC0-1.0. Véase `LICENSE`.
