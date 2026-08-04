# VPhysics provider demo made opt-in

The red falling shapes and the rotating floating box were not gameplay objects.
They were the provider-stack diagnostic scene created by
`blank3d_vphysics_spawn_demo()` at normal engine startup.

Normal builds now keep the demo disabled:

- no IDs 70001..70005 are spawned at startup;
- `blank3d_vphysics_update_demo()` is not called each frame;
- generic VPhysics debug geometry is not rendered;
- VPhysics itself remains enabled for shells and future physical objects.

To run the diagnostic scene intentionally:

```sh
make physics-demo
./blank3d_vphysics_demo.exe
```

The `B` key still pauses/resumes the shared physics service. In the explicit demo
build, `P` resets the diagnostic scene and `O` impulses object 70001.
