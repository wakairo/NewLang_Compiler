add_executable(p1592_probe tests/p1592/probe.c)
target_include_directories(p1592_probe PRIVATE src)
target_link_libraries(p1592_probe PRIVATE newlang_checked_c_node newlang_semantic)
target_link_options(p1592_probe PRIVATE -Wl,--wrap=malloc -Wl,--wrap=nl_h_custody_seal_result)
add_test(NAME p1592_native_and_boundaries.unit COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1592/run.py" "$<TARGET_FILE:newlangc>"
    "$<TARGET_FILE:p1592_probe>" "${CMAKE_CURRENT_BINARY_DIR}/p1592-native")
set_tests_properties(p1592_native_and_boundaries.unit PROPERTIES TIMEOUT 180)
add_test(NAME p1592_identity.unit COMMAND p1592_probe identity
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1592/source/identity-actuals.nl")
foreach(mode inner-oom outer-oom staging-oom)
    add_test(NAME p1592_${mode}.unit COMMAND p1592_probe ${mode}
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1592/source/direct-actuals.nl")
endforeach()
