foreach(target owned oom)
    add_executable(p1236_${target} tests/p1236/${target}.c)
    target_include_directories(p1236_${target} PRIVATE src)
    target_compile_options(p1236_${target} PRIVATE -UNDEBUG)
    target_link_libraries(p1236_${target} PRIVATE newlang_checked_c_node)
endforeach()
target_compile_definitions(p1236_owned PRIVATE P1236_EMIT_SAFETY)
target_compile_options(p1236_oom PRIVATE -fno-pie)
target_link_options(p1236_oom PRIVATE -no-pie
    -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc)
add_test(NAME p1236_source_native.integration COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1236/source_native.py"
    "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:p1236_owned>"
    "${CMAKE_C_COMPILER}" "${NEWLANG_SANITIZER}"
    "${CMAKE_CURRENT_BINARY_DIR}/p1236-source-native")
set_tests_properties(p1236_source_native.integration PROPERTIES RUN_SERIAL TRUE TIMEOUT 180)
add_test(NAME p1236_oom.unit COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1236/oom_gate.py"
    "$<TARGET_FILE:p1236_oom>" "${CMAKE_CURRENT_BINARY_DIR}/p1236-oom")
set_tests_properties(p1236_oom.unit PROPERTIES RUN_SERIAL TRUE TIMEOUT 300)
add_test(NAME p1236_history_negative.unit COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1236/history_negative.py")
set_tests_properties(p1236_history_negative.unit PROPERTIES RUN_SERIAL TRUE TIMEOUT 180)
