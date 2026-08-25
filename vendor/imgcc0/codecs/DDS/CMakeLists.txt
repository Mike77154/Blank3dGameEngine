cmake_minimum_required(VERSION 3.15)
project(giffany_dds_c89_static C)

enable_testing()
set(CMAKE_C_STANDARD 90)
set(CMAKE_C_STANDARD_REQUIRED YES)
set(CMAKE_C_EXTENSIONS NO)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic -pedantic-errors)
endif()

add_library(gdds STATIC
    src/gdds_common.c
    src/gdds_parse.c
    src/gdds_mips.c
    src/gdds_mipgen.c
    src/gdds_bc.c
    src/gdds_decode.c
    src/gdds_encode.c
    src/gdds_srgb.c
    src/gdds_normal.c
)
target_include_directories(gdds PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(gdds_example examples/minimal.c)
target_link_libraries(gdds_example PRIVATE gdds)

add_executable(gdds_mip_example examples/mipchain.c)
target_link_libraries(gdds_mip_example PRIVATE gdds)

add_executable(gdds_auto_mips_example examples/auto_mips.c)
target_link_libraries(gdds_auto_mips_example PRIVATE gdds)

add_executable(gdds_srgb_auto_mips_example examples/srgb_auto_mips.c)
target_link_libraries(gdds_srgb_auto_mips_example PRIVATE gdds)

add_executable(gdds_bc45_example examples/bc45.c)
target_link_libraries(gdds_bc45_example PRIVATE gdds)

add_executable(gdds_bc5_normal_example examples/bc5_normal_map.c)
target_link_libraries(gdds_bc5_normal_example PRIVATE gdds)

add_executable(gdds_bc5_snorm_conventions_example examples/bc5_snorm_conventions.c)
target_link_libraries(gdds_bc5_snorm_conventions_example PRIVATE gdds)

add_executable(gdds_compile_smoke tests/compile_smoke.c)
target_link_libraries(gdds_compile_smoke PRIVATE gdds)
add_test(NAME gdds_compile_smoke COMMAND gdds_compile_smoke)

add_executable(gdds_test_roundtrip tests/test_roundtrip.c)
target_link_libraries(gdds_test_roundtrip PRIVATE gdds)
add_test(NAME gdds_roundtrip COMMAND gdds_test_roundtrip)

add_executable(gdds_test_mipchain tests/test_mipchain.c)
target_link_libraries(gdds_test_mipchain PRIVATE gdds)
add_test(NAME gdds_mipchain COMMAND gdds_test_mipchain)

add_executable(gdds_test_auto_mips tests/test_auto_mips.c)
target_link_libraries(gdds_test_auto_mips PRIVATE gdds)
add_test(NAME gdds_auto_mips COMMAND gdds_test_auto_mips)

add_executable(gdds_test_srgb_mips tests/test_srgb_mips.c)
target_link_libraries(gdds_test_srgb_mips PRIVATE gdds)
add_test(NAME gdds_srgb_mips COMMAND gdds_test_srgb_mips)

add_executable(gdds_test_bc45 tests/test_bc45.c)
target_link_libraries(gdds_test_bc45 PRIVATE gdds)
add_test(NAME gdds_bc45 COMMAND gdds_test_bc45)

add_executable(gdds_test_bc5_normal tests/test_bc5_normal.c)
target_link_libraries(gdds_test_bc5_normal PRIVATE gdds)
add_test(NAME gdds_bc5_normal COMMAND gdds_test_bc5_normal)

add_executable(gdds_test_bc45_snorm tests/test_bc45_snorm.c)
target_link_libraries(gdds_test_bc45_snorm PRIVATE gdds)
add_test(NAME gdds_bc45_snorm COMMAND gdds_test_bc45_snorm)

add_executable(gdds_test_normal_conventions tests/test_normal_conventions.c)
target_link_libraries(gdds_test_normal_conventions PRIVATE gdds)
add_test(NAME gdds_normal_conventions COMMAND gdds_test_normal_conventions)

add_executable(gdds_test_byte_vectors tests/test_byte_vectors.c)
target_link_libraries(gdds_test_byte_vectors PRIVATE gdds)
add_test(NAME gdds_byte_vectors COMMAND gdds_test_byte_vectors)

add_executable(gdds_test_features tests/test_features.c)
target_link_libraries(gdds_test_features PRIVATE gdds)
add_test(NAME gdds_features COMMAND gdds_test_features)

add_executable(gdds_parity_harness tools/parity_harness.c)
target_link_libraries(gdds_parity_harness PRIVATE gdds)
