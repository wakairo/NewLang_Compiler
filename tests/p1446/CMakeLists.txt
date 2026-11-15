add_executable(p1446_probe tests/p1446/probe.c)
target_link_libraries(p1446_probe PRIVATE newlang_checked_c_node
    newlang_semantic newlang_parser)
target_link_options(p1446_probe PRIVATE -Wl,--wrap=malloc -Wl,--wrap=realloc
    -Wl,--wrap=nl_sem_clone -Wl,--wrap=nl_sem_function_exit)
add_test(NAME p1446_loan_exit.unit COMMAND p1446_probe diagnostic
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1441/N-loan-escape.nl"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1441/C-direct.nl"
    P8-EXIT-DEPENDENCY)
add_test(NAME p1446_internal_argument.unit COMMAND p1446_probe internal
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1441/C-direct.nl")
