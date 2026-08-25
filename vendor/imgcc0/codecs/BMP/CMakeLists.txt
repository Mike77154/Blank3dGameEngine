cmake_minimum_required(VERSION 3.16)
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" PROJECT_VERSION_FILE LIMIT_COUNT 1)
project(bmp VERSION ${PROJECT_VERSION_FILE} LANGUAGES C)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(CTest)
include(CheckCCompilerFlag)

option(BUILD_SHARED_LIBS "Build shared libraries" ON)

set(PREFIX ${CMAKE_INSTALL_PREFIX})
set(LIBDIR ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR})
set(INCLUDEDIR ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR})
set(VERSION ${PROJECT_VERSION})

option(BMP_BUILD_TESTS "Build BMP smoke tests" ON)
option(BMP_BUILD_TOOLS "Build BMP CLI/tools" ON)
option(BMP_ENABLE_MSVC_ANALYZE "Enable MSVC /analyze static analysis" OFF)
option(BMP_BUILD_FUZZ_C89 "Build strict C89 file/stdin fuzz harnesses" OFF)

set(BMP_SOURCES
    bmp/bmp_chunks.c
    bmp/bmp_metadata.c
    bmp/bmp_parser.c
    bmp/bmp_report.c
    bmp/bmp_decoder.c
    bmp/bmp_encoder.c
    bmp/bmp_filters.c
    bmp/bmp_render.c)

set(BMP_PUBLIC_HEADERS
    include/bmp/bmp.h
    include/bmp/bmp_export.h)

add_library(bmp ${BMP_SOURCES})
add_library(bmp::bmp ALIAS bmp)

set_target_properties(bmp PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    C_STANDARD 90
    C_STANDARD_REQUIRED YES
    C_EXTENSIONS NO
    POSITION_INDEPENDENT_CODE ON
    C_VISIBILITY_PRESET hidden
    WINDOWS_EXPORT_ALL_SYMBOLS OFF
    PUBLIC_HEADER "${BMP_PUBLIC_HEADERS}")

target_include_directories(bmp
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

function(bmp_append_supported_flag out_var flag)
    string(MAKE_C_IDENTIFIER "BMP${flag}" _flag_id)
    check_c_compiler_flag("${flag}" ${_flag_id})
    if(${${_flag_id}})
        set(${out_var} "${${out_var}};${flag}" PARENT_SCOPE)
    endif()
endfunction()

function(bmp_apply_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /WX /permissive- /external:anglebrackets /external:W0)
        if(BMP_ENABLE_MSVC_ANALYZE)
            target_compile_options(${target} PRIVATE /analyze)
        endif()
    else()
        set(BMP_WARNINGS -Wall -Wextra -Werror -pedantic -Wstrict-prototypes -Wmissing-prototypes -Wshadow -Wpointer-arith -Wformat=2 -Wundef)
        bmp_append_supported_flag(BMP_WARNINGS -Warray-bounds)
        bmp_append_supported_flag(BMP_WARNINGS -Wnull-dereference)
        if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
            bmp_append_supported_flag(BMP_WARNINGS -Wstringop-overread)
            bmp_append_supported_flag(BMP_WARNINGS -Wstringop-overflow=2)
            bmp_append_supported_flag(BMP_WARNINGS -Warray-bounds=2)
            bmp_append_supported_flag(BMP_WARNINGS -Wrestrict)
        endif()
        target_compile_options(${target} PRIVATE ${BMP_WARNINGS})
    endif()
endfunction()

bmp_apply_warnings(bmp)

if(BUILD_SHARED_LIBS)
    target_compile_definitions(bmp PRIVATE BMP_BUILDING_LIBRARY)
else()
    target_compile_definitions(bmp PUBLIC BMP_STATIC_DEFINE)
endif()
if(MSVC)
    set_property(TARGET bmp PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()
if(UNIX AND NOT APPLE)
    target_link_options(bmp PRIVATE
        "LINKER:-soname,libbmp.so.${PROJECT_VERSION_MAJOR}"
        "LINKER:--version-script=${CMAKE_CURRENT_SOURCE_DIR}/exports/bmp.map")
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/bmp.pc.in ${CMAKE_CURRENT_BINARY_DIR}/bmp.pc @ONLY)

if(BMP_BUILD_TESTS AND BUILD_TESTING)
    add_executable(bmp_smoke tests/test_smoke.c)
    target_link_libraries(bmp_smoke PRIVATE bmp)
    target_include_directories(bmp_smoke PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include)
    bmp_apply_warnings(bmp_smoke)
    add_test(NAME bmp_smoke COMMAND bmp_smoke)
    set_tests_properties(bmp_smoke PROPERTIES LABELS "smoke;api" WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

    add_executable(bmp_public_api_smoke tests/test_public_api.c)
    target_link_libraries(bmp_public_api_smoke PRIVATE bmp)
    target_include_directories(bmp_public_api_smoke PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    bmp_apply_warnings(bmp_public_api_smoke)
    add_test(NAME bmp_public_api_smoke COMMAND bmp_public_api_smoke)
    set_tests_properties(bmp_public_api_smoke PROPERTIES LABELS "smoke;api;public-header" WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif()

if(BMP_BUILD_TOOLS)
    add_executable(bmp_diag_cli tools/bmp_diag_cli.c)
    target_link_libraries(bmp_diag_cli PRIVATE bmp)
    target_include_directories(bmp_diag_cli PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    bmp_apply_warnings(bmp_diag_cli)
    add_executable(bmp_compat_cli tools/bmp_compat_cli.c)
    target_link_libraries(bmp_compat_cli PRIVATE bmp)
    target_include_directories(bmp_compat_cli PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    bmp_apply_warnings(bmp_compat_cli)
    add_executable(bmp_bench tools/bmp_bench.c)
    target_link_libraries(bmp_bench PRIVATE bmp)
    target_include_directories(bmp_bench PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    bmp_apply_warnings(bmp_bench)
endif()

if(BMP_BUILD_FUZZ_C89)
    add_executable(bmp_fuzz_c89_file tests/fuzz_c89_file.c)
    target_link_libraries(bmp_fuzz_c89_file PRIVATE bmp)
    target_include_directories(bmp_fuzz_c89_file PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    bmp_apply_warnings(bmp_fuzz_c89_file)
    add_executable(bmp_fuzz_c89_stdin tests/fuzz_c89_stdin.c)
    target_link_libraries(bmp_fuzz_c89_stdin PRIVATE bmp)
    target_include_directories(bmp_fuzz_c89_stdin PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    bmp_apply_warnings(bmp_fuzz_c89_stdin)
endif()

install(TARGETS bmp
    EXPORT bmpTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/bmp)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/bmp.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/bmpConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/bmpConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/bmpConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/bmp)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/bmpConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/bmpConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/bmp)
install(EXPORT bmpTargets
    NAMESPACE bmp::
    FILE bmpTargets.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/bmp)
set(CPACK_PACKAGE_NAME "bmp")
set(CPACK_PACKAGE_VENDOR "Codec Forge")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Industrial-grade BMP codec library")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_FILE_NAME "bmp-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "bmp-${PROJECT_VERSION}-source")
set(CPACK_GENERATOR "TGZ;ZIP")
set(CPACK_SOURCE_GENERATOR "TGZ;ZIP")
set(CPACK_VERBATIM_VARIABLES YES)
set(CPACK_SOURCE_IGNORE_FILES
    "/\\.git/"
    "/build-cmake/"
    "/build-cpack/"
    "/dist/"
    "/\\.release/"
    "/\\.cmake-stage/"
    "/\\.pkgconfig-stage/"
    "/tests/compat_reports/"
    "/tests/perf_reports/"
    "/tests/coverage_reports/"
    "/tests/policy_reports/"
    "/tests/governance_reports/"
    "/tests/package_reports/"
    "~$"
)
include(CPack)

