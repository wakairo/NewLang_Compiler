add_executable(p1654_probe tests/p1654/probe.c)
target_include_directories(p1654_probe PRIVATE src)
target_link_libraries(p1654_probe PRIVATE newlang_checked_c_node newlang_semantic)
target_link_options(p1654_probe PRIVATE -Wl,--wrap=malloc
    -Wl,--wrap=nl_h_custody_seal_result)

set(p1654_source "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1654/selection.nl")
add_test(NAME p1654_custody_rollback.unit COMMAND p1654_probe "${p1654_source}")
add_test(NAME p1654_native_scope.unit COMMAND "${Python3_EXECUTABLE}" -B
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1654/run.py"
    "$<TARGET_FILE:newlangc>" "${CMAKE_CURRENT_BINARY_DIR}/p1654-native" "${p1654_source}")
