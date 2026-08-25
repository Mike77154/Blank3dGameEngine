cmake_minimum_required(VERSION 3.10)
project(condor_evact C)

option(CONDOR_EVACT_WITH_INPUT_BRIDGE "Build InputScanner bridge" OFF)
option(CONDOR_EVACT_BUILD_TESTS "Build standalone self-test" ON)

add_library(condor_evact STATIC
    actioncore.c
    condcore.c
    evcore.c
    evact.c
)

set_target_properties(condor_evact PROPERTIES
    C_STANDARD 90
    C_STANDARD_REQUIRED YES
    C_EXTENSIONS NO
)

target_include_directories(condor_evact PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

if(CONDOR_EVACT_WITH_INPUT_BRIDGE)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../input_scanner/input_ev_handler.h")
        message(FATAL_ERROR "CONDOR_EVACT_WITH_INPUT_BRIDGE=ON but ../input_scanner/input_ev_handler.h is missing")
    endif()

    if(NOT TARGET input_scanner)
        message(FATAL_ERROR "CONDOR_EVACT_WITH_INPUT_BRIDGE=ON but target input_scanner is not available")
    endif()

    target_sources(condor_evact PRIVATE evcore_input_bridge.c)
    target_include_directories(condor_evact PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${CMAKE_CURRENT_SOURCE_DIR}/../input_scanner
    )
    target_link_libraries(condor_evact PUBLIC input_scanner)
endif()

if(CONDOR_EVACT_BUILD_TESTS)
    enable_testing()
    add_executable(condor_evact_selftest tests/selftest.c)
    set_target_properties(condor_evact_selftest PROPERTIES
        C_STANDARD 90
        C_STANDARD_REQUIRED YES
        C_EXTENSIONS NO
    )
    target_link_libraries(condor_evact_selftest PRIVATE condor_evact)
    add_test(NAME condor_evact_selftest COMMAND condor_evact_selftest)
endif()
