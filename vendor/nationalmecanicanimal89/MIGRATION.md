# Migración desde Mechanimer89 y NRPA3D

## Desde Mechanimer89

| Mechanimer89 | NationalMecanicanimal89 |
|---|---|
| `mech89_rig` | `nm89_rig` |
| `mech89_part.local` | `nm89_part.manual` |
| `mech89_part.evaluated` | `nm89_pose.resolved` |
| límites dentro de part | banco `nm89_constraint` |
| pivot dentro de transform | banco `nm89_pivot` o pivot provider |
| `mech89_geometry_binding.bind` | `nm89_alignment` |
| sequence/track | clip/track/key |
| socket provider único | varios providers registrados |
| transform provider único | varios providers registrados |
| geometry provider | `apply_geometry` |

La conversión recomendada crea primero constraints, pivotes y alignments; después crea parts y bindings con sus IDs.

## Desde NRPA3D

| NRPA3D | NationalMecanicanimal89 |
|---|---|
| `nrpa3d_rig` | `nm89_rig` |
| `nrpa3d_limits` | `nm89_constraint` |
| `nrpa3d_part.home` | `nm89_part.home` |
| `nrpa3d_pose.transform` | `nm89_pose.resolved` |
| object/group único | uno o varios `nm89_binding` |
| event from/to | tracks con keys por canal |
| provider apply_part | provider `apply_geometry` |
| jerarquía delegada | jerarquía interna con opción de world provider |

Los clips NRPA3D que contienen una transformación completa deben dividirse en tracks sólo para los canales que realmente cambian.
