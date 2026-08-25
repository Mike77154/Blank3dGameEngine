# Building with CMake

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
cmake --install build --prefix /tmp/pcx-stage
```

Consumer projects can then do:

```cmake
find_package(pcx CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE pcx::pcx)
```

A ready-made consumer smoke test lives under `examples/cmake_consumer` and can be executed with:

```sh
make cmake-smoke
```
