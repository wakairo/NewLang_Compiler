add_executable(p1393_owned tests/p1393/owned.c)
target_link_libraries(p1393_owned PRIVATE newlang_checked_c_node newlang_semantic newlang_parser)
add_test(NAME p1393_owned.unit COMMAND p1393_owned owned "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1393/sources/P1-C-free-current-tail-read-v2.nl")
target_link_options(p1393_owned PRIVATE -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc
    -Wl,--wrap=nl_captured_read_begin -Wl,--wrap=nl_captured_read_end)
add_test(NAME p1393_temporal_oom.unit COMMAND p1393_owned oom "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1393/sources/P1-C-free-current-tail-read-v2.nl")
