add_executable(p1598_probe tests/p1598/probe.c)
target_include_directories(p1598_probe PRIVATE src)
target_link_libraries(p1598_probe PRIVATE newlang_checked_c_node newlang_semantic)
target_link_options(p1598_probe PRIVATE -Wl,--wrap=malloc -Wl,--wrap=nl_h_custody_seal_result)

add_test(NAME p1598_formal_copy_native.unit COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1598/run.py" "$<TARGET_FILE:newlangc>"
    "$<TARGET_FILE:p1596_probe>" "$<TARGET_FILE:p1598_probe>"
    "${CMAKE_CURRENT_BINARY_DIR}/p1598-native")
set_tests_properties(p1598_formal_copy_native.unit PROPERTIES TIMEOUT 180)

foreach(mode identity leaf-oom entry-oom staging-oom)
    add_test(NAME p1598_${mode}.unit COMMAND p1598_probe ${mode}
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1598/source/copy-final-return.nl" carry)
endforeach()
