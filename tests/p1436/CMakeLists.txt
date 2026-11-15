add_executable(p1436_probe tests/p1436/probe.c)
target_link_libraries(p1436_probe PRIVATE newlang_checked_c_node
    newlang_semantic newlang_parser)
target_link_options(p1436_probe PRIVATE -Wl,--wrap=malloc
    -Wl,--wrap=realloc -Wl,--wrap=nl_sem_clone
    -Wl,--wrap=nl_h_custody_seal_result)
add_test(NAME p1436_world_auth.unit COMMAND p1436_probe auth
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1435/P-some-bool.nl"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1435/P-none-bool-copy.nl")
add_test(NAME p1436_snapshot_oom.unit COMMAND p1436_probe snapshot-oom
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1435/P-some-bool.nl")
add_test(NAME p1436_cli_native.integration COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1436/native.py"
    "$<TARGET_FILE:newlangc>" "${CMAKE_C_COMPILER}"
    "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/p1436-native")
set_tests_properties(p1436_cli_native.integration PROPERTIES TIMEOUT 180)
