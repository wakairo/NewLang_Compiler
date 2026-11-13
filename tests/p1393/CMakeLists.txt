add_executable(p1393_owned tests/p1393/owned.c)
target_link_libraries(p1393_owned PRIVATE newlang_checked_c_node newlang_semantic newlang_parser)
add_test(NAME p1393_owned.unit COMMAND p1393_owned "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1393/sources/P1-C-free-current-tail-read-v2.nl")
