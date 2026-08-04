cmake_minimum_required(VERSION 3.10)

add_library(polls STATIC
    key_pc.c
)

target_include_directories(polls PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}        # permite: "key_pc.h"
    ${CMAKE_CURRENT_SOURCE_DIR}/..     # permite: "polls/key_pc.h"
)

# Nota:
# polls ya NO tiene dependencia hardcodeada a "input_scanner".
# Si quieres usar input_scanner, compila/usa el adapter (binding) fuera de este target.
