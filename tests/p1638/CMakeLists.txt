add_executable(p1638_probe tests/p1638/probe.c)
target_include_directories(p1638_probe PRIVATE src)
target_link_libraries(p1638_probe PRIVATE newlang_checked_c_node newlang_semantic)
target_link_options(p1638_probe PRIVATE -Wl,--wrap=malloc
    -Wl,--wrap=nl_h_custody_seal_result)

set(p1638_source
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1638/source/early-return.nl")
add_test(NAME p1638_native.unit COMMAND "${Python3_EXECUTABLE}" -B
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1638/run.py" "$<TARGET_FILE:newlangc>"
    "${CMAKE_CURRENT_BINARY_DIR}/p1638-native" early-return "${p1638_source}")
add_test(NAME p1638_scope.unit COMMAND "${Python3_EXECUTABLE}" -B
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1638/run.py" "$<TARGET_FILE:newlangc>"
    "${CMAKE_CURRENT_BINARY_DIR}/p1638-scope" scope
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1638/source/two-usize-early-return.nl"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1638/source/two-usize-normal-if.nl")
set_tests_properties(p1638_native.unit p1638_scope.unit PROPERTIES TIMEOUT 180)

foreach(mode identity result-oom staging-oom)
    add_test(NAME p1638_${mode}.unit COMMAND p1638_probe ${mode}
        "${p1638_source}")
    set_tests_properties(p1638_${mode}.unit PROPERTIES TIMEOUT 180)
endforeach()
