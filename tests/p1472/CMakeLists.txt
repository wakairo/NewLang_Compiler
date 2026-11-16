add_executable(p1472_probe tests/p1472/probe.c)
target_include_directories(p1472_probe PRIVATE src)
target_link_libraries(p1472_probe PRIVATE newlang_checked_c_node newlang_semantic newlang_parser)
target_link_options(p1472_probe PRIVATE -Wl,--wrap=malloc -Wl,--wrap=realloc -Wl,--wrap=nl_sem_clone)
add_test(NAME p1472_terminal_carrier_auth_negative.unit COMMAND p1472_probe suite
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1463"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1471"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1472")
