add_executable(p1572_probe tests/p1572/probe.c)
target_include_directories(p1572_probe PRIVATE src)
target_link_libraries(p1572_probe PRIVATE newlang_checked_c_node newlang_semantic)
target_link_options(p1572_probe PRIVATE -Wl,--wrap=malloc
    -Wl,--wrap=nl_h_custody_seal_result -Wl,--wrap=nl_usize_arithmetic_capture)
foreach(mode positive negative identity second-leaf-oom outer-nodes-oom outer-world-oom staging-oom)
    add_test(NAME p1572_${mode}.unit COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1572/run.py" ${mode}
        "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:p1572_probe>"
        "${CMAKE_CURRENT_BINARY_DIR}/p1572-${mode}")
    set_tests_properties(p1572_${mode}.unit PROPERTIES TIMEOUT 180)
endforeach()
