# GProj2D89 1.0

Biblioteca de siluetas vectoriales 2D para munición de videojuegos y HUDs, escrita en C89 estricto y con números Q16.16.

## Contrato técnico

- C89.
- Sin asignación dinámica dentro de la biblioteca.
- Sin tipos numéricos de punto flotante.
- Sin dependencia de OpenGL, DirectX, SDL, Win32 ni del sistema operativo.
- El caller entrega arreglos estáticos para `paths` y `points`.
- La geometría se entrega como polígonos y líneas; el renderer final sigue perteneciendo al engine.
- Colores RGBA8 por path.
- Outline activable/desactivable, recoloreable y con grosor configurable.
- Transformación Q16.16 con posición, escala no uniforme y rotación en 64 pasos.

## Arquitectura

```text
ammo_id / perfil
       |
       +---- shell_shape_id --------+
       |                             |
       +---- projectile_shape_id ---+----> generadores de primitivas
                                            |
                                            v
                                  gp2d_scene del caller
                                  paths + points + roles
                                            |
                                            v
                                  renderer del engine / SVG / HUD
```

La identidad de la munición no está fundida con la geometría. `GP2D_AMMO_SNIPER`, por ejemplo, selecciona un casquillo abotellado y un proyectil spitzer con boat-tail, pero ambos se pueden pedir por separado mediante `GP2D_PART_SHELL` y `GP2D_PART_PROJECTILE`.

## Catálogo incluido

| Perfil | Shell | Proyectil |
|---|---|---|
| Pistola | casquillo recto | gota/round nose estilizada |
| Escopeta / postas | bloque rojo con base amarilla y primer | grupo de 9 círculos |
| Escopeta / slug | bloque rojo con base amarilla y primer | slug de punta plana |
| Ametralladora | casquillo abotellado | spitzer |
| Magnum | casquillo recto con rim | round nose con banda |
| Rifle sniper | casquillo abotellado largo | spitzer con boat-tail |
| Misil | no aplica | nariz, cuerpo, banda, aletas y nozzle |
| Granada de mano | no aplica | cuerpo esférico, espoleta, palanca y anillo |
| Lanzagranadas | casing corto | granada redondeada con banda y base |

## Primitivas públicas

- círculo;
- elipse;
- cápsula vertical u horizontal;
- polígono arbitrario;
- gota;
- round nose;
- spitzer;
- spitzer con boat-tail;
- flat point.

El catálogo construye shapes complejos combinando esas primitivas, pero puedes usarlas directamente para crear perfiles nuevos.

## Uso mínimo

```c
#include "gproj2d89.h"

#define MAX_PATHS 64
#define MAX_POINTS 1024

static gp2d_path paths[MAX_PATHS];
static gp2d_vec2 points[MAX_POINTS];

int build_sniper_projectile(void)
{
    gp2d_scene scene;
    gp2d_transform transform;
    gp2d_draw_options options;

    gp2d_scene_init(&scene, paths, MAX_PATHS, points, MAX_POINTS);

    transform = gp2d_transform_identity();
    transform.position.x = gp2d_fx_from_int(320L);
    transform.position.y = gp2d_fx_from_int(180L);
    transform.scale.x = gp2d_fx_from_int(64L);
    transform.scale.y = gp2d_fx_from_int(64L);

    options = gp2d_draw_options_default();
    options.outline_enabled = GP2D_TRUE;
    options.outline_width = gp2d_fx_from_int(2L);

    return gp2d_build_part(&scene,
                           GP2D_AMMO_SNIPER,
                           GP2D_PART_PROJECTILE,
                           &transform,
                           &options);
}
```

## Recolor por rol

Cada path lleva un rol semántico (`GP2D_ROLE_SHELL_BODY`, `GP2D_ROLE_PROJECTILE`, `GP2D_ROLE_BAND`, `GP2D_ROLE_FIN`, etc.). Eso permite cambiar sólo una región:

```c
gp2d_scene_recolor_role(&scene,
                         GP2D_ROLE_PROJECTILE,
                         gp2d_color_rgba(80U, 220U, 255U, 255U));
```

También existen:

- `gp2d_scene_recolor_all`;
- `gp2d_scene_set_outline`;
- override de fill/outline desde `gp2d_draw_options`.

## Compilación y pruebas

```sh
make
make test
make audit
make preview
```

`make preview` recompila el demo SVG y rasteriza el PNG. La biblioteca y el demo C no necesitan librerías gráficas; sólo la conversión SVG→PNG usa el script Python incluido.

## Preview

- `preview/gproj2d89_preview.svg`
- `preview/gproj2d89_preview.png`

El preview se genera desde la API real de la biblioteca. No es un mock dibujado aparte.

## Alcance de realismo

Las siluetas están diseñadas para ser reconocibles y coherentes en HUDs, inventarios, pickups y sprites vectoriales. No son planos de fabricación ni sustituyen dibujos dimensionales de SAAMI/CIP. Las proporciones son genéricas para evitar acoplar el catálogo a un calibre o fabricante concreto.

## Fuentes de diseño consultadas

- SAAMI, definición de cartucho y glosario técnico: https://saami.org/glossary/cartridge/ y https://saami.org/saami-glossary/
- SAAMI, estándares y dibujos de cartucho/cámara: https://saami.org/technical-information/ansi-saami-standards/
- Hornady, anatomía de bala y diferencias entre flat base y boat-tail: https://www.hornady.com/bullets/anatomy-of-a-bullet
- Hornady, perfiles secant ogive y boat-tail: https://www.hornady.com/bullets/sst-%28super-shock-tip%29
- U.S. Army PEO Ammunition, granadas letales M67/M213: https://www.cpeae.army.mil/Project-Offices/PM-CCS/Organizations/PdD-Combat-Armaments-and-Protection-Systems/Products/Grenades/Lethal-Hand-Grenades/
- Winchester, shotshells con hull, head, primer, wad y cargas de shot/slug: https://winchester.com/Products/Ammunition/Shotshell/AA y https://winchester.com/-/media/PDFs/Safety-Data-Sheets/CARTRIDGES---SHOTSHELL/CARTRIDGES---SHOTSHELL.ashx

## Licencia

CC0-1.0. Consulta `LICENSE`.
