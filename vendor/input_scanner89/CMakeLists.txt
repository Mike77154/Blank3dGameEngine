cmake_minimum_required(VERSION 3.10)
project(input_scanner C)

set(INPUT_SCANNER_SOURCES
    input_scanner.c
    input_ev_handler.c
    input_backend_mask.c
    input_backend_keyboard.c
    input_backend_mouse.c
    input_backend_pad.c
)

add_library(input_scanner STATIC ${INPUT_SCANNER_SOURCES})

target_include_directories(input_scanner
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

if (MSVC)
    target_compile_options(input_scanner PRIVATE /W4)
else()
    target_compile_options(input_scanner PRIVATE -std=c89 -pedantic -Wall -Wextra -Werror)
endif()

add_executable(input_scanner_smoke_test test_compile.c)
target_link_libraries(input_scanner_smoke_test PRIVATE input_scanner)

if (MSVC)
    target_compile_options(input_scanner_smoke_test PRIVATE /W4)
else()
    target_compile_options(input_scanner_smoke_test PRIVATE -std=c89 -pedantic -Wall -Wextra -Werror)
endif()
