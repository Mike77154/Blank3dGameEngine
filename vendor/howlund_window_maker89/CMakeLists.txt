cmake_minimum_required(VERSION 3.10)
project(howlund_window_maker89 C)

add_library(howm89 STATIC src/howm89.c)
target_include_directories(howm89 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

option(HOWM89_BUILD_DEFAULT_BACKEND "Build an optional default OS backend" OFF)
option(HOWM89_BUILD_TESTS "Build HOWM89 core tests" ON)

if(HOWM89_BUILD_DEFAULT_BACKEND)
    if(WIN32)
        add_library(howm89_backend STATIC
            backends/win32/howm89_win32_backend.c
            backends/auto/howm89_auto_backend.c)
        target_include_directories(howm89_backend PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${CMAKE_CURRENT_SOURCE_DIR}/backends/win32
            ${CMAKE_CURRENT_SOURCE_DIR}/backends/auto)
        target_link_libraries(howm89_backend PUBLIC howm89)
    elseif(APPLE)
        enable_language(OBJC)
        add_library(howm89_backend STATIC
            backends/cocoa/howm89_cocoa_backend.m
            backends/auto/howm89_auto_backend.c)
        target_include_directories(howm89_backend PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${CMAKE_CURRENT_SOURCE_DIR}/backends/cocoa
            ${CMAKE_CURRENT_SOURCE_DIR}/backends/auto)
        target_link_libraries(howm89_backend PUBLIC howm89 "-framework Cocoa")
    elseif(UNIX)
        find_package(X11 REQUIRED)
        add_library(howm89_backend STATIC
            backends/x11/howm89_x11_backend.c
            backends/auto/howm89_auto_backend.c)
        target_include_directories(howm89_backend PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${CMAKE_CURRENT_SOURCE_DIR}/backends/x11
            ${CMAKE_CURRENT_SOURCE_DIR}/backends/auto
            ${X11_INCLUDE_DIR})
        target_link_libraries(howm89_backend PUBLIC howm89 ${X11_LIBRARIES})
    endif()
endif()

if(HOWM89_BUILD_TESTS)
    add_executable(test_howm89 tests/test_howm89.c)
    target_link_libraries(test_howm89 PRIVATE howm89)
endif()
