# Bibliography and visual references

## Museum measurements

- Metropolitan Museum of Art, *Blade for a Sword (Katana)*, object 36.25.1664a-c.
  https://www.metmuseum.org/art/collection/search/24341
- Metropolitan Museum of Art, *Art of the Samurai: Japanese Arms and Armor, 1156–1868*.
  https://www.metmuseum.org/exhibitions/listings/2009/art-of-the-samurai/photo-gallery
- British Museum, katana 1958,0730.141 (70.10 cm blade; 2.40 cm curvature).
  https://www.britishmuseum.org/collection/object/A_1958-0730-141
- British Museum, katana 1958,0730.134 (60.30 cm cutting edge; 1 cm curvature).
  https://www.britishmuseum.org/collection/object/A_1958-0730-134-a-e
- British Museum, katana 1878,1230.838 (65 cm cutting edge; 1.20 cm curvature).
  https://www.britishmuseum.org/collection/object/A_1878-1230-838-a-d
- British Museum, Kambun-shape katana 1958,0730.67.
  https://www.britishmuseum.org/collection/object/A_1958-0730-67-a-b
- e-Museum / National Institutes for Cultural Heritage, unsigned katana by Norifusa.
  https://emuseum.nich.go.jp/detail?content_base_id=101342&langId=en
- Metropolitan Museum of Art tsuba collection examples.
  https://www.metmuseum.org/art/collection/search/30064
  https://www.metmuseum.org/art/collection/search/35210

## Terminology

- Nihonto.com glossary (hamon, hi, hira-zukuri, etc.).
  https://nihonto.com/about-swords/glossary/
- British Museum object records for tsuka, tsuba, saya, ray skin, braid and fittings.
  https://www.britishmuseum.org/collection/object/A_1958-0730-188-a-d

## Low-poly topology reference

- Blender Manual, bevel edges and custom split normals.
  https://docs.blender.org/manual/en/latest/modeling/meshes/editing/edge/bevel.html

## Design notes

The implementation uses compact convex strips and extrusions rather than texture-
dependent detail. Hamon, bohi, yokote and sukashi are low-cost visual cues. The
silhouette, blade curvature, taper, kissaki length, guard profile and grip ratio
carry most of the recognition at gameplay distance.
