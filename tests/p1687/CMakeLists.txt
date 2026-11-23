set(p1687_sources "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1687/source")
add_executable(p1687_probe tests/p1687/probe.c)
target_link_libraries(p1687_probe PRIVATE newlang_checked_c_node newlang_semantic)
add_test(NAME p1687_local_product_custody.unit COMMAND p1687_probe
    "${p1687_sources}/copy-chain.nl")
add_test(NAME p1687_cli_native.integration COMMAND "${Python3_EXECUTABLE}" -B
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1687/run.py"
    "$<TARGET_FILE:newlangc>" "${p1687_sources}")
add_test(NAME p1687_local_product_rollback.unit COMMAND p1673_probe
    local-product-oom "${p1687_sources}/copy-tail.nl" unused)
set_tests_properties(p1687_local_product_rollback.unit PROPERTIES TIMEOUT 90)
