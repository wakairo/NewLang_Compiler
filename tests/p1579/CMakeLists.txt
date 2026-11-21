add_executable(p1579_probe tests/p1579/probe.c)
target_include_directories(p1579_probe PRIVATE src)
target_link_libraries(p1579_probe PRIVATE newlang_checked_c_node newlang_semantic)
target_link_options(p1579_probe PRIVATE -Wl,--wrap=malloc
    -Wl,--wrap=nl_h_custody_seal_result -Wl,--wrap=nl_usize_arithmetic_capture)
foreach(mode positive negative identity nested-result-oom outer-nodes-oom outer-world-oom staging-oom)
    add_test(NAME p1579_${mode}.unit COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1579/run.py" ${mode}
        "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:p1579_probe>"
        "${CMAKE_CURRENT_BINARY_DIR}/p1579-${mode}")
    set_tests_properties(p1579_${mode}.unit PROPERTIES TIMEOUT 180)
endforeach()
