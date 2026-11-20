add_executable(p1554_probe tests/p1554/probe.c)
target_include_directories(p1554_probe PRIVATE src)
target_link_libraries(p1554_probe PRIVATE newlang_checked_c_node newlang_semantic)
target_link_options(p1554_probe PRIVATE -Wl,--wrap=malloc -Wl,--wrap=calloc
    -Wl,--wrap=nl_h_custody_seal_result -Wl,--wrap=nl_usize_arithmetic_capture)
foreach(mode positive negative authenticity receipt-oom result-oom world-oom staging-oom)
    add_test(NAME p1554_${mode}.unit COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1554/run.py" ${mode}
        "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:p1554_probe>"
        "${CMAKE_CURRENT_BINARY_DIR}/p1554-${mode}")
    set_tests_properties(p1554_${mode}.unit PROPERTIES TIMEOUT 180)
endforeach()
