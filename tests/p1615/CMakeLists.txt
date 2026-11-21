add_executable(p1615_probe tests/p1615/probe.c)
target_include_directories(p1615_probe PRIVATE src)
target_link_libraries(p1615_probe PRIVATE newlang_checked_c_node newlang_semantic)
target_link_options(p1615_probe PRIVATE -Wl,--wrap=malloc
    -Wl,--wrap=nl_sem_bind_in_scope -Wl,--wrap=nl_h_custody_seal_result)

add_test(NAME p1615_multi_argument_native.unit COMMAND "${Python3_EXECUTABLE}" -B
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1615/run.py" "$<TARGET_FILE:newlangc>"
    "$<TARGET_FILE:p1615_probe>" "${CMAKE_CURRENT_BINARY_DIR}/p1615-native")
set_tests_properties(p1615_multi_argument_native.unit PROPERTIES TIMEOUT 180)

foreach(mode identity second-formal-oom result-seal-oom c17-staging-oom)
    add_test(NAME p1615_${mode}.unit COMMAND p1615_probe "${mode}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1615/source/evaluated-unused.nl")
endforeach()
