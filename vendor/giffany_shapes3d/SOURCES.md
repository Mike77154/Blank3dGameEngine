# Technical sources

These references informed the library's API and query design. The code is an original
fixed-point C89 implementation and does not copy source code from these documents.

1. **Khronos Group, OpenGL 4.6 Core Profile Specification**, Chapter 10,
   “Vertex Specification and Drawing Commands.” It describes geometric primitives as
   sequences of vertices carrying generic vertex attributes and assembled into points,
   lines, or triangles.

   https://registry.khronos.org/OpenGL/specs/gl/glspec46.core.pdf

2. **Tomas Moller and Ben Trumbore, “Fast, Minimum Storage Ray/Triangle
   Intersection.”** The paper derives the ray/triangle test that returns ray distance and
   barycentric coordinates without storing a plane equation per triangle. `g3d_ray_triangle`
   follows that mathematical method, rewritten for saturating Q16.16 arithmetic.

   https://cadxfem.org/inf/Fast%20MinimumStorage%20RayTriangle%20Intersection.pdf

3. **David Eberly, Geometric Tools Documentation.** Reference material for point/segment
   distance, line and ray queries, and separating-axis methods.

   https://www.geometrictools.com/Documentation/Documentation.html

4. **RaiSim collision documentation.** Its primitive-pair table explicitly describes
   sphere/capsule as closest point on the capsule axis followed by a sphere/sphere test,
   and capsule/capsule as a segment/segment closest-approach problem.

   https://raisim.com/sections/CollisionDetection.html
