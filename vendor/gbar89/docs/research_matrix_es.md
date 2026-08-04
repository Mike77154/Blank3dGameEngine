# Matriz de investigación para GBAR89 v0.3

La extensión v0.3 traduce ideas comunes de motores y APIs modernas a un subconjunto C89, fixed-point y backend-agnóstico.

| Referencia | Idea tomada | Traducción en GBAR89 |
|---|---|---|
| Godot `TextureProgressBar` | capas under/progress/over, fill horizontal/vertical/radial, nine-patch | fondo, valor, overlay, direcciones, radial y nine-slice ya presentes; v0.3 agrega decoración procedural alrededor |
| Unity UI `Image` | simple/sliced/tiled/filled, fill origin, radial wipes | direcciones existentes, máscaras, fondos repetidos procedurales y cuantización del wipe |
| Unreal `UProgressBar` | fill por escala o máscara, padding, restyling y marquee | máscara/escala existentes, marcos y efectos reestilizables; marquee queda como ampliación futura |
| SDL render geometry | triángulos indexados y color por vértice | máscaras, radial y glifos vectoriales mediante `draw_triangles` |
| SDL clip rectangle | recorte por destino | fondos, efectos y máscaras respetan `push_clip/pop_clip` cuando el backend los ofrece |
| SVG | vectores 2D, patrones, máscaras, filtros visuales | glifos normalizados, patrones rectangulares, máscaras poligonales y falsos filtros construidos con primitivas 2D |
| WCAG uso de color | no depender únicamente del color | patrones, scanlines, grid, pips y glifos permiten codificar estados también por forma/textura |
| Pixel-art `image-rendering` | conservar celdas y bordes duros | `GBAR89_FLAG_PIXEL_QUANTIZE` y `GBAR89_FX_PIXEL_CELLS` |

## Ampliaciones compatibles con el diseño actual

Estas ideas caben sin romper la API base y pueden añadirse como módulos posteriores:

- gradientes por bandas o por color de vértice;
- marquee/indeterminate y desplazamiento de patrones por fase fixed-point;
- rampas de color por umbrales múltiples;
- texto/números mediante callback de glifos del backend;
- marcos vectoriales personalizados;
- unidades parcialmente llenas;
- varios canales apilados en una sola barra;
- distorsión de onda mediante slices enteros;
- caché de command buffers estáticos para estilos que no cambian;
- exportador de presets a un DSL de estilo.

## Referencias oficiales consultadas

- https://docs.godotengine.org/en/stable/classes/class_textureprogressbar.html
- https://docs.unity3d.com/462/Documentation/Manual/script-Image.html
- https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/UMG/Components/UProgressBar
- https://wiki.libsdl.org/SDL3/SDL_RenderGeometry
- https://wiki.libsdl.org/SDL3/SDL_SetRenderClipRect
- https://developer.mozilla.org/en-US/docs/Web/SVG
- https://developer.mozilla.org/en-US/docs/Web/CSS/Reference/Properties/image-rendering
- https://www.w3.org/WAI/WCAG22/Understanding/use-of-color.html


## Investigación añadida para v0.4

| Fuente primaria | Idea trasladada a GBAR89 |
|---|---|
| Godot `TextureProgressBar` | Un solo medidor puede compartir rellenos horizontal, vertical y radial; el radial expone ángulo inicial, amplitud y centro. GBAR89 conserva inicio/amplitud y extiende el mismo compositor a las tres orientaciones. |
| Unity `Image.FillMethod` / `Radial360` | Separar cantidad visible, método de llenado y origen. GBAR89 mantiene el ratio lógico separado de geometría, dirección y barrido. |
| Unreal `UProgressBar` | Diferenciar relleno por escala y por máscara. GBAR89 conserva rutas rectangulares/masked y genera la ruta polar mediante triángulos. |
| W3C SVG Strokes | Caps y patrones discontinuos son propiedades independientes del trazo. GBAR89 añade caps butt/round/square y segmentos/gaps angulares. |

### Decisiones propias de implementación

- No se añadió shader, stencil obligatorio ni textura polar.
- Los fondos polares se construyen con arcos, bandas y líneas.
- Los caps redondos son discos triangulados; los cuadrados son quads orientados con la LUT trigonométrica entera.
- El `mid_value` radial dibuja únicamente el intervalo entre dos ratios, no vuelve a pintar todo el anillo.
- `radial_phase_deg` desplaza detalles procedurales para animación sin cambiar el valor lógico.
