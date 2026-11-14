add_executable(p1426_qualified tests/p1426/qualified.c)
target_link_libraries(p1426_qualified PRIVATE newlang_checked_c_node
    newlang_semantic newlang_parser)
target_link_options(p1426_qualified PRIVATE -Wl,--wrap=malloc)

add_test(NAME p1426_authentic.unit COMMAND p1426_qualified positive
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1420/U.nl"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1420/Alias.nl"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1426/False.nl")
add_test(NAME p1426_fail_closed.unit COMMAND p1426_qualified faults
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1420/U.nl")
add_test(NAME p1426_emitter_oom.unit COMMAND p1426_qualified oom
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1420/U.nl")
add_test(NAME p1426_cli_native.integration COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1426/native.py"
    "$<TARGET_FILE:newlangc>" "${CMAKE_C_COMPILER}"
    "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/p1426-native")
set_tests_properties(p1426_cli_native.integration PROPERTIES TIMEOUT 180)
