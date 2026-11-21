add_executable(p1623_probe tests/p1623/probe.c)
target_include_directories(p1623_probe PRIVATE src)
target_link_libraries(p1623_probe PRIVATE newlang_checked_c_node newlang_semantic)
target_link_options(p1623_probe PRIVATE -Wl,--wrap=malloc
    -Wl,--wrap=nl_sem_bind_in_scope -Wl,--wrap=nl_h_custody_seal_result)

add_test(NAME p1623_typed_arguments_native.unit COMMAND "${Python3_EXECUTABLE}" -B
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1623/run.py" "$<TARGET_FILE:newlangc>"
    "$<TARGET_FILE:p1623_probe>" "${CMAKE_CURRENT_BINARY_DIR}/p1623-native")
set_tests_properties(p1623_typed_arguments_native.unit PROPERTIES TIMEOUT 180)

foreach(mode identity bool-formal-oom mixed-seal-oom entry-world-oom staging-oom)
    add_test(NAME p1623_${mode}.unit COMMAND p1623_probe "${mode}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1623/source/equal-typed.nl")
endforeach()
