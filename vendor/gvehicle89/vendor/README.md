# gvehicle89 movement subvendors — Vehicle System phase 1

`gvehicle89` remains the stable facade. Movement/physics policy is now split below it:

- `vehicleprovider89`: optional per-vehicle/provider dispatch for movement modules and physics phases.

- `vehiclephysics89`: common gravity, mass-point contacts, drag, integration.
- `carmovement89`: wheels, suspension, tires, drivetrain and ground steering.
- `tankmovement89`: tracked/tank movement boundary around the current tank4 movement solver.
- `watermovement89`: buoyancy, water drag, thrust and yaw.
- `airmovement89`: fixed-wing forces plus common air-assist application.
- `rotorcraftmovement89`: rotor force/control application.
- `spacemovement89`: spacecraft/6DoF movement application.
- `motorcyclemovement89`: specialization seam; phase 1 delegates to car movement and is not dispatched yet.
- `busmovement89`: specialization seam; phase 1 delegates to car movement and is not dispatched yet.

The facade still owns gameplay modules such as nitro, kudos, missions, avionics and damage.
Mounting/occupancy remains outside this tree (`gvehpos89` + `3d_mounting_system89`).
