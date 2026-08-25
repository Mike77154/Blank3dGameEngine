cmake_minimum_required(VERSION 3.10)
project(rt_time C)

add_library(rt_time STATIC
    rt_time.c
)

target_include_directories(rt_time
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

option(RT_TIME_BUILD_TESTS "Build rt_time tests" OFF)

if(RT_TIME_BUILD_TESTS)
    enable_testing()

    add_executable(rt_time_tests
        tests/test_rt_time.c
    )

    target_link_libraries(rt_time_tests
        PRIVATE
            rt_time
    )

    add_test(NAME rt_time_tests COMMAND rt_time_tests)
endif()
