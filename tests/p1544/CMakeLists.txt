add_executable(p1544_probe tests/p1544/probe.c)
target_link_libraries(p1544_probe PRIVATE newlang_checked_c_node newlang_semantic)
target_link_options(p1544_probe PRIVATE -Wl,--wrap=malloc -Wl,--wrap=calloc
    -Wl,--wrap=nl_usize_arithmetic_capture -Wl,--wrap=nl_h_custody_seal_result)
foreach(mode positive50 positive0 zero range profile authenticity unknown receipt-oom result-oom emitter-oom native controls)
    add_test(NAME p1544_${mode}.unit COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1544/run.py" ${mode}
        "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:p1544_probe>"
        "${CMAKE_C_COMPILER}" "${CMAKE_CURRENT_BINARY_DIR}/p1544-${mode}")
    set_tests_properties(p1544_${mode}.unit PROPERTIES TIMEOUT 180 RUN_SERIAL TRUE)
endforeach()
