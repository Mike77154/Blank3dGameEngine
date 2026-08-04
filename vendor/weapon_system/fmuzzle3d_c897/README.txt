FMUZZLE89 v6 - corrected 2 cm + 18 cm core proportions
========================================================

Purpose
-------
A bounded procedural 3D muzzle-flash geometry generator for strict C89 engines.
It uses Q10 fixed point, fixed-size arrays and no dynamic allocation.

Core geometry
-------------
The warm-white axial body is now:

    2 cm cylinder + 18 cm cone = 20 cm total

The cylinder and the cone base share exactly the same diameter:

    diameter = 2.5 cm
    radius   = 1.25 cm

The cone starts immediately at z = 2 cm, so there is no gap or diameter step.
Both the core and fin interiors use the same warm-white render colour. Only the
fin outlines are yellow.

Fin anchor
----------
Both fin banks still grow from the rear/base ring of the cylinder:

    anchor_z      = 0 cm
    anchor_radius = 1.25 cm

The primary star and its interleaved repeated star share that same base ring.

Repeater rule
-------------
The primary star contains N fins. The repeated star adds N fins at the angular
midpoints. Each repeated fin inherits its parent shape and roll and defaults to
exactly half the parent length and half the parent width.

Example used by the first preview shot:

    5 primary triangles: 5.00 cm long, 2.50 cm full width
    5 repeated triangles: 2.50 cm long, 1.25 cm full width
    total: 10 visible fins

Temporarily enabled profiles
----------------------------
Enabled:

    FM89_SHAPE_TRIANGLE
    FM89_SHAPE_LEAF

Reserved but disabled for generation:

    FM89_SHAPE_BOX

Exact core setup
----------------
    FM89_Config config;

    fm89_default_config(&config);
    config.cylinder_length = FM89_CM(2);
    config.cone_length = FM89_CM(18);
    config.core_radius = FM89_MM(25) / 2L;

The complete diameter is therefore:

    config.core_radius * 2 == FM89_MM(25)

Build
-----
MSYS2 / MinGW32:

    make
    make test
    make preview

Or run build_mingw32.bat. The demo writes PPM frames using its own strict-C89
fixed-point software renderer. The supplied PNG and GIF files are assembled
directly from those C-generated frames.

Preview note: triangle fins are rendered with an open-root outline in demo_render.c. The yellow contour is removed from the lower base edge while the fins still originate from the cylinder base ring.

This build adds a cone-base crown: original cylinder-base star + cylinder repeater + cone-base star + cone-base repeater. Default deterministic preview uses 5 primaries at 5 cm, 5 cylinder repeaters at 2.5 cm, 5 cone-base primaries at 2.5 cm, and 5 cone-base repeaters at 1.25 cm.
