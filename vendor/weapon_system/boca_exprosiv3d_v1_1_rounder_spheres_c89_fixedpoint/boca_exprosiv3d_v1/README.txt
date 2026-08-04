boca_exprosiv3d v1.1
=====================

Biblioteca C89 autocontenida para crear explosiones de boca 3D procedurales
mediante mallas de primitivas low-poly.

No modifica ni depende de fmuzzle89. Ambas bibliotecas pueden convivir:

- fmuzzle89: destello estilizado de aletas.
- boca_exprosiv3d: chorro de gases inspirado en la física.

REGLAS CUMPLIDAS
----------------

- C89 estricto.
- Sin malloc.
- Sin realloc.
- Sin free.
- Sin heap interno.
- Sin float.
- Sin double.
- Sin texturas.
- Sin sprites.
- Sin billboards.
- Sin shaders obligatorios.
- Todo Q10 fixed-point.
- Toda la geometría son triángulos.
- El buffer de malla pertenece al caller y puede ser estático/reutilizable.

MODELO VISUAL
-------------

No resuelve CFD ni pretende predecir un arma real con exactitud forense. Es un
modelo visual determinista que convierte parámetros físicos normalizados en una
aproximación low-poly de las regiones más reconocibles de un muzzle blast:

1. Primary jet
   Un frustum que nace en el diámetro del ánima y se expande hacia delante.

2. Shock cells
   Pares de frustums convergentes/divergentes que forman celdas de compresión y
   expansión.

3. Mach disks
   Discos finos con tapas, colocados dentro de las primeras celdas.

4. Vortex rings
   Toros de 10 x 4 segmentos que avanzan y se expanden.

5. Secondary flash
   Esferas low-poly separadas de la boca cuando queda combustible residual que
   se mezcla con el aire exterior.

6. Gas clouds
   Esferas elipsoidales de baja opacidad que sobreviven al núcleo brillante.

7. Muzzle-device jets
   Frustums dirigidos según apagallamas, freno horizontal, freno radial,
   supresor o fuga lateral de revólver.

La separación entre flash primario y combustión secundaria se inspira en
mediciones donde la segunda región aparece más adelante, al mezclarse gases de
propelente con oxígeno. Las celdas, discos y vórtices se inspiran en estudios de
chorros de boca subexpandidos y su estructura de ondas/vorticidad.

Referencias de diseño:

- https://www.osti.gov/biblio/7238764
- https://www.mdpi.com/2226-4310/11/5/381
- https://www.mdpi.com/2076-3417/10/4/1468

PERFILES INCLUIDOS
------------------

BEX3D_PROFILE_PISTOL
    Compacto; una o dos celdas; flash secundario moderado.

BEX3D_PROFILE_MACHINE_GUN
    Perfil de cartucho de rifle con apagallamas; el estado térmico de una
    ráfaga debe manejarlo el arma/engine, no esta explosión individual.

BEX3D_PROFILE_SHOTGUN
    Mayor expansión radial, más turbulencia y más nubes de gas.

BEX3D_PROFILE_MAGNUM_AUTO
    Presión, masa de gas y combustible residual altos.

BEX3D_PROFILE_MAGNUM_REVOLVER
    Perfil Magnum más dos jets laterales por la separación cilindro-cañón.

BEX3D_PROFILE_PRECISION_RIFLE
    Sesgo axial fuerte, celdas más numerosas y apagallamas.

BEX3D_PROFILE_PRECISION_BRAKE
    Perfil de precisión con dos chorros laterales grandes de freno de boca.

BEX3D_PROFILE_SUPPRESSED
    Presión visual, combustible, radio y presupuesto muy reducidos.

USO MINIMO
----------

    #include "boca_exprosiv3d.h"

    #define MY_MAX_VERTICES 512
    #define MY_MAX_INDICES  4096

    static BEX3D_State muzzle;
    static BEX3D_Vertex vertices[MY_MAX_VERTICES];
    static unsigned short indices[MY_MAX_INDICES];
    static BEX3D_MeshBuffer mesh;

    void weapon_init(void)
    {
        BEX3D_Profile profile;

        bex3d_init(&muzzle, 0x47494646UL);
        bex3d_default_profile(&profile, BEX3D_PROFILE_PISTOL);
        bex3d_set_profile(&muzzle, &profile);

        mesh.vertices = vertices;
        mesh.indices = indices;
        mesh.vertex_capacity = MY_MAX_VERTICES;
        mesh.index_capacity = MY_MAX_INDICES;
    }

    void weapon_fire(void)
    {
        bex3d_fire(&muzzle);
    }

    void weapon_update(unsigned long elapsed_us)
    {
        bex3d_update_us(&muzzle, elapsed_us);
    }

    void weapon_draw(void)
    {
        int result;

        result = bex3d_build_mesh(&muzzle, &mesh);
        if (result == BEX3D_BUILD_OK)
        {
            engine_draw_additive_mesh(mesh.vertices,
                                      mesh.vertex_count,
                                      mesh.indices,
                                      mesh.index_count);
        }
    }

TRANSFORM DEL ARMA
------------------

La explosión se genera sobre +Z local. Para colocarla en cualquier arma:

    bex3d_set_origin(&muzzle, muzzle_x, muzzle_y, muzzle_z);

    bex3d_set_basis(&muzzle,
                    right_x, right_y, right_z,
                    up_x, up_y, up_z,
                    forward_x, forward_y, forward_z);

Los tres ejes deben llegar ortonormales y expresados en Q10. La biblioteca no los
normaliza para evitar raíz cuadrada y costo innecesario.

RENDER RECOMENDADO
------------------

- Depth test: activado.
- Depth write: desactivado durante el efecto.
- Blend: aditivo para jet, celdas, discos, anillos y flash secundario.
- Las nubes de gas pueden dibujarse con alpha tradicional si el renderer separa
  por rol; si se usa una sola pasada, el aditivo tenue sigue siendo aceptable.
- Culling: desactivado o double-sided para evitar huecos desde ángulos extremos.
- No requiere iluminación: cada vértice ya contiene RGBA emisivo.

La API principal entrega una sola malla. Para un renderer que quiera separar
roles, puede recorrer bex3d_get_primitive() y construir adaptadores propios.

PRESUPUESTO MEDIDO
------------------

Barrido de cada preset cada 100 microsegundos con la semilla de la demo:

- Pistola:              117 vértices, 178 triángulos.
- Ametralladora:        302 vértices, 459 triángulos.
- Escopeta:             245 vértices, 358 triángulos.
- Magnum automática:   311 vértices, 468 triángulos.
- Magnum revólver:      329 vértices, 472 triángulos.
- Rifle de precisión:   288 vértices, 425 triángulos.
- Rifle con freno:      361 vértices, 504 triángulos.
- Suprimida:            100 vértices, 167 triángulos.

La versión 1.0 cabía dentro de 512 vértices y 4096 índices en los presets incluidos.
La versión 1.1 con esferas más redondas recomienda 1024 vértices y 8192 índices
para evitar truncado en rifle con freno y Magnum revólver.

MEMORIA
-------

La biblioteca no reserva memoria oculta.

En ABI Win32/MinGW32 esperado, con long de 32 bits:

- BEX3D_Profile:     68 bytes.
- BEX3D_Primitive: 108 bytes.
- BEX3D_State:    4,456 bytes.
- BEX3D_Vertex:      16 bytes.

Buffer compartido recomendado para los presets incluidos:

- 512 vértices x 16 bytes = 8,192 bytes.
- 4096 índices x 2 bytes  = 8,192 bytes.
- Total scratch mesh       = 16 KiB compartidos.

El estado por arma puede reducirse bajando BEX3D_MAX_PRIMITIVES antes de compilar,
siempre que también reduzcas primitive_budget en los perfiles personalizados.

TIEMPO
------

bex3d_update_us() usa microsegundos enteros. El evento físico dura aproximadamente
entre 3 y 5 ms según el preset. En una pantalla de 60 Hz puede desaparecer entre
frames; el engine puede congelar una muestra visible durante un frame, mientras
la edad física interna sigue siendo microsegundos.

DETERMINISMO
------------

La misma semilla, perfil y orden de llamadas generan los mismos descriptores de
malla. Esto permite replays, demos sincronizadas y depuración reproducible.

ARCHIVOS
--------

boca_exprosiv3d.h
    API pública.

boca_exprosiv3d.c
    Generación física, primitivas y constructor de malla.

test_boca_exprosiv3d.c
    Suite estricta C89.

demo_export.c
    Exporta una muestra OBJ de cada perfil usando sólo fixed-point.

mesh_budget.c
    Barre el tiempo y reporta el pico de geometría de cada perfil.

samples/*.obj
    Mallas generadas a 900 microsegundos.

COMPILAR
--------

MSYS2/MinGW:

    make
    make test
    make samples
    make budget

Windows CMD:

    build_mingw32.bat

LICENCIA
--------

CC0 1.0. Puedes vendorizar, modificar, renombrar y distribuir la biblioteca.
