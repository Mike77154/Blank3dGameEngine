get_filename_component(CODEC_SOURCE_PATH "${CURRENT_PORT_DIR}/../../../../" ABSOLUTE)

if(NOT EXISTS "${CODEC_SOURCE_PATH}/VERSION")
    message(FATAL_ERROR "Expected local source checkout at ${CODEC_SOURCE_PATH}")
endif()

vcpkg_cmake_configure(
    SOURCE_PATH "${CODEC_SOURCE_PATH}"
    OPTIONS
        -DBMP_BUILD_TESTS=OFF
        -DBMP_BUILD_TOOLS=OFF
)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup(PACKAGE_NAME bmp CONFIG_PATH lib/cmake/bmp)
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(INSTALL "${CURRENT_PORT_DIR}/copyright" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")
