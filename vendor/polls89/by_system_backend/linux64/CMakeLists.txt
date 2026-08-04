# Backend Linux 32-bit (evdev) para teclado PC

add_library(linuxpckeys_backend STATIC
    linuxpckeys_backend.c
)

target_include_directories(linuxpckeys_backend PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../..        # polls/polls
    ${CMAKE_CURRENT_SOURCE_DIR}/../../..     # polls/ (permite "polls/key_pc.h")
)

target_link_libraries(linuxpckeys_backend PUBLIC polls)
