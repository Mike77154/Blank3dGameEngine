# gbulletmesh89

C89 fixed-point mesh helper for tiny 3D bullets, shells, cartridges, pellets and machine-gun belt links.

## Constraints

- C89 / gnu89 friendly.
- No malloc, realloc, free, heap, float or double in the library.
- Caller-owned vertex and triangle buffers.
- Fixed point: Q16.16 signed `long`.
- Mesh output is agnostic: the demo writes OBJ only as a convenience.

## Ammo families

| Type | Shell mesh | Projectile mesh | Visual logic |
|---|---|---|---|
| `GBM_AMMO_PISTOL` | straight brass case + rim | round-nose bullet | common semi-auto pistol silhouette |
| `GBM_AMMO_SHOTGUN` | wide red hull + brass head + crimp | loose low-poly pellets | readable buckshot/shot effect |
| `GBM_AMMO_SNIPER` | bottleneck rifle case | long spitzer with boat-tail | precision/rifle silhouette |
| `GBM_AMMO_MAGNUM` | chunkier rimmed straight case | semi-wadcutter / hollow hint | revolver-magnum flavor |
| `GBM_AMMO_MACHINEGUN` | bottleneck rifle-like case | compact spitzer with boat-tail | belt-fed rifle/MG ammo flavor |

## API

```c
GBM_Vertex verts[512];
GBM_Tri tris[1024];
GBM_Mesh mesh;

gbm_mesh_init(&mesh, verts, 512, tris, 1024);
gbm_build_shell(&mesh, GBM_AMMO_PISTOL);
gbm_build_projectile(&mesh, GBM_AMMO_SNIPER);
gbm_build_shotgun_pellet(&mesh); /* one gameplay pellet */
gbm_build_full_round(&mesh, GBM_AMMO_MACHINEGUN);
gbm_build_mg_link(&mesh);
```

## Demo

From the package root:

```sh
gcc -std=c89 -pedantic -Wall -Wextra -Isrc src/gbulletmesh89.c demo/demo_bulletmesh89.c -o demo_bulletmesh89
./demo_bulletmesh89
```

It writes OBJ files into `obj/`.

## Mesh budget notes

The default radial budget is 12 segments. If you want a PS1-ish chunkier look, set `GBM_SEGMENTS` and the tables to 8. For prettier close-up items, keep 12 or build a 16-segment variant.
