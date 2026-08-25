# Web research notes — v0.4.0

The expansion was guided by public standards/documentation, without copying protected artwork:

- W3C SVG 2 Basic Shapes: a compact path-oriented substrate can represent rectangles, circles, ellipses, lines, polylines and polygons.
- W3C SVG Markers: markers may occur at start/end/vertices/segments or repeat along a path; this motivated `connectors/markers` and repeatable marker semantics.
- Graphviz Node Shapes: demonstrates a broad reusable diagram-shape vocabulary including polygonal nodes, cylinders, notes, tabs, folders, components and record-like nodes.
- OMG BPMN 2.0.2: distinguishes events, activities, gateways, flows, data objects and related process-diagram concepts; our `diagrams/process` recipes are deliberately simplified generic geometry, not a claim of BPMN conformance.
- IEC 60617: maintains graphical symbols for diagrams as structured data sheets and motivated expanding `technical_symbols`; recipes here are generic/simplified and are not certified IEC symbols.
- Blender curve documentation: Poly, Bezier, NURBS and Catmull-Rom are distinct spline families; this motivated a broader mathematical/spline-oriented curve vocabulary while retaining the stable path-command ABI.
- NOAA/NWS surface-map material shows established visual families such as high/low pressure, station plots, fronts, isobars and wind barbs; our weather recipes are simplified engine primitives, not operational meteorological chart products.

No renderer dependency was introduced.
