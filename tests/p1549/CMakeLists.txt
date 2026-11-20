add_executable(p1549_probe tests/p1549/probe.c)
target_include_directories(p1549_probe PRIVATE src)
target_link_libraries(p1549_probe PRIVATE newlang_checked_c_node newlang_semantic)
target_link_options(p1549_probe PRIVATE -Wl,--wrap=malloc
    -Wl,--wrap=nl_h_custody_seal_result -Wl,--wrap=nl_usize_arithmetic_capture)
foreach(mode positive negative authenticity second-seal world-seal staging)
    add_test(NAME p1549_${mode}.unit COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1549/run.py" ${mode}
        "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:p1549_probe>"
        "${CMAKE_CURRENT_BINARY_DIR}/p1549-${mode}")
endforeach()
