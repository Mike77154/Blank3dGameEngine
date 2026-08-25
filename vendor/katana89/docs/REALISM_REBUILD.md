# katana89 v1.1 realism rebuild

The mesh generator was rebuilt around measured museum examples rather than a generic fantasy-sword silhouette.

Reference range used for the presets:

- Metropolitan Museum katana cutting edge: 70.2-70.6 cm; curvature: 1.4-2.1 cm.
- British Museum examples: 60.3-71.4 cm cutting edges; curvature: 1.0-2.4 cm.
- Kyoto National Museum / ColBase example: 69.4 cm edge; 2.0 cm curvature.
- British Museum Kambun example: shallow curve and marked narrowing toward the point.
- British Museum mokko tsuba: 8.5 x 8.1 cm.

Geometry decisions:

1. Ko/chu/o kissaki occupy short/medium/long terminal regions rather than a generic long triangle.
2. The ha and mune are generated independently, so the point can sweep naturally into the kissaki.
3. Shinogi-zukuri uses an eight-point cross-section with a distinct ridge and mune shoulder.
4. The tsuba uses real inner-wall topology around an open nakago-ana.
5. The tsuka uses a rounded-rectangular section with paired crossing wraps.

Sources:

- https://www.metmuseum.org/art/collection/search/24341
- https://www.metmuseum.org/art/collection/search/21811
- https://www.metmuseum.org/art/collection/search/23987
- https://www.britishmuseum.org/collection/object/A_1958-0730-134-a-e
- https://www.britishmuseum.org/collection/object/A_1958-0730-141
- https://www.britishmuseum.org/collection/object/A_1958-0730-164-a-d
- https://www.britishmuseum.org/collection/object/A_1958-0730-67-a-b
- https://www.britishmuseum.org/collection/object/A_TS-47
- https://colbase.nich.go.jp/collection_items/kyohaku/E%E7%94%B2181
