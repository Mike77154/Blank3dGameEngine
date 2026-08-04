# Backend Win32 para el sistema de teclado PC (winpckeys_backend)

add_library(winpckeys_backend STATIC
    winpckeys_backend.c
)

target_include_directories(winpckeys_backend PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../..        # permite includes directos si se requiere
    ${CMAKE_CURRENT_SOURCE_DIR}/../../..     # permite: "polls/key_pc.h"
)

target_link_libraries(winpckeys_backend PUBLIC polls user32)








