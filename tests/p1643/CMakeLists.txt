add_executable(p1643_probe tests/p1643/probe.c)
target_include_directories(p1643_probe PRIVATE src)
target_link_libraries(p1643_probe PRIVATE newlang_checked_c_node
    newlang_semantic)
target_link_options(p1643_probe PRIVATE -Wl,--wrap=malloc
    -Wl,--wrap=nl_h_custody_seal_result
    -Wl,--wrap=nl_control_quantity_return_receipt_issue)

set(p1643_source
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1638/source/early-return.nl")
foreach(mode authenticity slot-substitution receipt-failures)
    add_test(NAME p1643_${mode}.unit COMMAND p1643_probe ${mode}
        "${p1643_source}")
    set_tests_properties(p1643_${mode}.unit PROPERTIES TIMEOUT 180)
endforeach()

add_test(NAME p1643_scope.unit COMMAND "${Python3_EXECUTABLE}" -B
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1643/scope.py"
    "$<TARGET_FILE:newlangc>" "${CMAKE_CURRENT_BINARY_DIR}/p1643-scope"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1643/source/reversed-early-return.nl"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1643/source/unsupported-one-return.nl")
set_tests_properties(p1643_scope.unit PROPERTIES TIMEOUT 180)
