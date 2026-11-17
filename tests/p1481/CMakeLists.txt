add_executable(p1481_probe tests/p1481/probe.c)
target_include_directories(p1481_probe PRIVATE src)
target_link_libraries(p1481_probe PRIVATE newlang_checked_c_node newlang_semantic newlang_parser)
target_link_options(p1481_probe PRIVATE -Wl,--wrap=malloc -Wl,--wrap=realloc -Wl,--wrap=nl_sem_clone)
set(p1481_base "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1481")
add_test(NAME p1481_conditional_current_owned.unit COMMAND p1481_probe owned
    "${p1481_base}/P1468-B2-after-cleanup-in-success-arm.nl" 0 181)
add_test(NAME p1481_success_zero_owned.unit COMMAND p1481_probe owned
    "${p1481_base}/Z-success-zero.nl" 0 0)
add_test(NAME p1481_index_one_owned.unit COMMAND p1481_probe owned
    "${p1481_base}/I-field-index-one.nl" 1 181)
add_test(NAME p1481_conditional_current_safety.unit COMMAND p1481_probe suite
    "${p1481_base}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1463")
add_test(NAME p1481_conditional_current_native.integration
    COMMAND "${Python3_EXECUTABLE}" "${p1481_base}/native.py"
    "$<TARGET_FILE:newlangc>" "${CMAKE_C_COMPILER}" "${p1481_base}"
    "${CMAKE_CURRENT_BINARY_DIR}/p1481-native" "${NEWLANG_SANITIZER}")
set_tests_properties(p1481_conditional_current_safety.unit
    p1481_conditional_current_native.integration PROPERTIES TIMEOUT 180 RUN_SERIAL TRUE)
