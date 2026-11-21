add_executable(p1596_probe tests/p1596/probe.c)
target_include_directories(p1596_probe PRIVATE src)
target_link_libraries(p1596_probe PRIVATE newlang_checked_c_node newlang_semantic)
target_link_options(p1596_probe PRIVATE -Wl,--wrap=malloc -Wl,--wrap=nl_h_custody_seal_result)

add_test(NAME p1596_native_and_boundaries.unit COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1596/run.py" "$<TARGET_FILE:newlangc>"
    "$<TARGET_FILE:p1596_probe>" "${CMAKE_CURRENT_BINARY_DIR}/p1596-native")
set_tests_properties(p1596_native_and_boundaries.unit PROPERTIES TIMEOUT 180)

foreach(mode identity actual-oom entry-oom staging-oom)
    add_test(NAME p1596_${mode}.unit COMMAND p1596_probe ${mode}
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1596/source/direct-main.nl")
endforeach()
