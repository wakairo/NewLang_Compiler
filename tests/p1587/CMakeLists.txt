add_executable(p1587_probe tests/p1587/probe.c)
target_include_directories(p1587_probe PRIVATE src)
target_link_libraries(p1587_probe PRIVATE newlang_checked_c_node newlang_semantic)
target_link_options(p1587_probe PRIVATE -Wl,--wrap=malloc -Wl,--wrap=realloc
    -Wl,--wrap=nl_if_field_plan -Wl,--wrap=nl_sem_new_value
    -Wl,--wrap=nl_h_custody_seal_result)
foreach(mode native negative identity join-oom result-oom staging-oom)
    add_test(NAME p1587_${mode}.unit COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1587/run.py" ${mode}
        "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:p1587_probe>"
        "${CMAKE_CURRENT_BINARY_DIR}/p1587-${mode}")
    set_tests_properties(p1587_${mode}.unit PROPERTIES TIMEOUT 180)
endforeach()
