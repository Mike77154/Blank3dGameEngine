# Backend macOS 32-bit para teclado PC

add_library(macpckeys_backend STATIC
    macpckeys_backend.c
)

target_include_directories(macpckeys_backend PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../..        # polls/polls
    ${CMAKE_CURRENT_SOURCE_DIR}/../../..     # polls/ (permite "polls/key_pc.h")
)

target_link_libraries(macpckeys_backend PUBLIC
    polls
    "-framework ApplicationServices"
    "-framework Carbon"
)
