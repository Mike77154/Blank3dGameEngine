# Adapter/binding: polls <-> input_scanner

add_library(polls_input_scanner_binding STATIC
    polls_input_scanner_binding.c
)

target_include_directories(polls_input_scanner_binding PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../..        # polls/ (permite "polls/..."
    ${CMAKE_CURRENT_SOURCE_DIR}/../..           # polls/polls
)

target_link_libraries(polls_input_scanner_binding PUBLIC
    polls
    input_scanner
)
