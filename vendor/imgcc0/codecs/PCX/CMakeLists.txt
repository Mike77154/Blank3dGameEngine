cmake_minimum_required(VERSION 3.16)
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" PROJECT_VERSION_FILE LIMIT_COUNT 1)
project(pcx VERSION ${PROJECT_VERSION_FILE} LANGUAGES C)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(CTest)
include(CheckCCompilerFlag)

option(BUILD_SHARED_LIBS "Build shared libraries" ON)

set(PREFIX ${CMAKE_INSTALL_PREFIX})
set(LIBDIR ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR})
set(INCLUDEDIR ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR})
set(VERSION ${PROJECT_VERSION})

option(PCX_BUILD_TESTS "Build PCX smoke tests" ON)
option(PCX_BUILD_TOOLS "Build PCX CLI/tools" ON)
option(PCX_ENABLE_MSVC_ANALYZE "Enable MSVC /analyze static analysis" OFF)

set(PCX_SOURCES
    pcx_chunk.c
    pcx_parser.c
    pcx_render.c
    pcx_decoder.c
    pcx_encoder.c
    pcx_report.c)

set(PCX_PUBLIC_HEADERS
    include/pcx/pcx.h
    include/pcx/pcx_export.h
    include/pcx/pcx_config89.h)

add_library(pcx ${PCX_SOURCES})
add_library(pcx::pcx ALIAS pcx)

set_target_properties(pcx PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    C_STANDARD 90
    C_STANDARD_REQUIRED YES
    C_EXTENSIONS NO
    POSITION_INDEPENDENT_CODE ON
    C_VISIBILITY_PRESET hidden
    PUBLIC_HEADER "${PCX_PUBLIC_HEADERS}")

target_include_directories(pcx
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

function(pcx_append_supported_flag out_var flag)
    string(MAKE_C_IDENTIFIER "PCX${flag}" _flag_id)
    check_c_compiler_flag("${flag}" ${_flag_id})
    if(${${_flag_id}})
        set(${out_var} "${${out_var}};${flag}" PARENT_SCOPE)
    endif()
endfunction()

function(pcx_apply_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /WX /permissive- /external:anglebrackets /external:W0)
        if(PCX_ENABLE_MSVC_ANALYZE)
            target_compile_options(${target} PRIVATE /analyze)
        endif()
    else()
        set(PCX_WARNINGS -Wall -Wextra -Werror -pedantic -Wstrict-prototypes -Wmissing-prototypes -Wshadow -Wpointer-arith -Wformat=2 -Wundef)
        pcx_append_supported_flag(PCX_WARNINGS -Warray-bounds)
        pcx_append_supported_flag(PCX_WARNINGS -Wnull-dereference)
        if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
            pcx_append_supported_flag(PCX_WARNINGS -Walloc-zero)
            pcx_append_supported_flag(PCX_WARNINGS -Wmismatched-dealloc)
            pcx_append_supported_flag(PCX_WARNINGS -Wstringop-overread)
            pcx_append_supported_flag(PCX_WARNINGS -Wstringop-overflow=2)
            pcx_append_supported_flag(PCX_WARNINGS -Warray-bounds=2)
            pcx_append_supported_flag(PCX_WARNINGS -Wrestrict)
        endif()
        target_compile_options(${target} PRIVATE ${PCX_WARNINGS})
    endif()
endfunction()

pcx_apply_warnings(pcx)

if(BUILD_SHARED_LIBS)
    target_compile_definitions(pcx PRIVATE PCX_BUILDING_LIBRARY)
else()
    target_compile_definitions(pcx PUBLIC PCX_STATIC_DEFINE)
endif()
if(MSVC)
    set_property(TARGET pcx PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()
if(UNIX AND NOT APPLE)
    target_link_options(pcx PRIVATE
        "LINKER:-soname,libpcx.so.${PROJECT_VERSION_MAJOR}"
        "LINKER:--version-script=${CMAKE_CURRENT_SOURCE_DIR}/exports/pcx.map")
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/pcx.pc.in ${CMAKE_CURRENT_BINARY_DIR}/pcx.pc @ONLY)

if(PCX_BUILD_TESTS AND BUILD_TESTING)
    add_executable(pcx_smoke tests/test_smoke.c)
    target_link_libraries(pcx_smoke PRIVATE pcx)
    target_include_directories(pcx_smoke PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include)
    pcx_apply_warnings(pcx_smoke)
    add_test(NAME pcx_smoke COMMAND pcx_smoke)
    set_tests_properties(pcx_smoke PROPERTIES LABELS "smoke;api" WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

    add_executable(pcx_public_api_smoke tests/test_public_api.c)
    target_link_libraries(pcx_public_api_smoke PRIVATE pcx)
    target_include_directories(pcx_public_api_smoke PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    pcx_apply_warnings(pcx_public_api_smoke)
    add_test(NAME pcx_public_api_smoke COMMAND pcx_public_api_smoke)
    set_tests_properties(pcx_public_api_smoke PROPERTIES LABELS "smoke;api;public-header" WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif()

if(PCX_BUILD_TOOLS)
    add_executable(pcx_diag_cli tools/pcx_diag_cli.c)
    target_link_libraries(pcx_diag_cli PRIVATE pcx)
    target_include_directories(pcx_diag_cli PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    pcx_apply_warnings(pcx_diag_cli)
    add_executable(pcx_bench tools/pcx_bench.c)
    target_link_libraries(pcx_bench PRIVATE pcx)
    target_include_directories(pcx_bench PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    pcx_apply_warnings(pcx_bench)
endif()

install(TARGETS pcx
    EXPORT pcxTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/pcx)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/pcx.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/pcxConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/pcxConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/pcxConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/pcx)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/pcxConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/pcxConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/pcx)
install(EXPORT pcxTargets
    NAMESPACE pcx::
    FILE pcxTargets.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/pcx)

set(CPACK_PACKAGE_NAME "pcx")
set(CPACK_PACKAGE_VENDOR "Codec Forge")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Industrial-grade PCX codec library")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_FILE_NAME "pcx-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "pcx-${PROJECT_VERSION}-source")
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

