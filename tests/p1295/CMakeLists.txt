add_executable(p1295_prefix tests/p1295/prefix.c)
target_include_directories(p1295_prefix PRIVATE src)
target_compile_options(p1295_prefix PRIVATE -UNDEBUG)
target_link_libraries(p1295_prefix PRIVATE newlang_checked_c_node)
add_executable(p1295_emit_current tests/p1295/prefix.c)
target_include_directories(p1295_emit_current PRIVATE src)
target_compile_options(p1295_emit_current PRIVATE -UNDEBUG)
target_compile_definitions(p1295_emit_current PRIVATE P1295_EMIT=1)
target_link_libraries(p1295_emit_current PRIVATE newlang_checked_c_node)
add_test(NAME p1295_prefix.unit COMMAND p1295_prefix
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1295/P-surviving-state.nl")
add_test(NAME p1295_emit_current.unit COMMAND p1295_emit_current
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1295/P-surviving-state.nl")
add_test(NAME p1295_copy_owned.unit COMMAND p1291_emit_owned
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1295/P-surviving-state.nl" 71 163 179 1)
add_test(NAME p1295_native.integration COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1295/native.py"
    "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:p1291_owned>"
    "$<TARGET_FILE:p1295_prefix>" "${CMAKE_C_COMPILER}" "${NEWLANG_SANITIZER}"
    "${CMAKE_CURRENT_BINARY_DIR}/p1295-native")
add_test(NAME p1295_negative_output.unit COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1295/failure.py" negative
    "$<TARGET_FILE:p1291_oom>" "$<TARGET_FILE:newlangc>"
    "${CMAKE_CURRENT_BINARY_DIR}/p1295-negative")
add_test(NAME p1295_check_oom.unit COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1295/failure.py" check
    "$<TARGET_FILE:p1291_check_oom>" "$<TARGET_FILE:newlangc>"
    "${CMAKE_CURRENT_BINARY_DIR}/p1295-check-oom")
add_test(NAME p1295_emit_oom.unit COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1295/failure.py" emit
    "$<TARGET_FILE:p1291_oom>" "$<TARGET_FILE:newlangc>"
    "${CMAKE_CURRENT_BINARY_DIR}/p1295-emit-oom")
add_test(NAME p1295_named_oom.integration COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1295/failure.py" named
    "$<TARGET_FILE:p1280_cli_oom>" "$<TARGET_FILE:newlangc>"
    "${CMAKE_CURRENT_BINARY_DIR}/p1295-named-oom")
set_tests_properties(p1295_check_oom.unit p1295_emit_oom.unit
    p1295_named_oom.integration PROPERTIES RUN_SERIAL TRUE)
cmake_language(DEFER CALL set_tests_properties
    p1295_check_oom.unit p1295_emit_oom.unit p1295_named_oom.integration
    PROPERTIES TIMEOUT 300)
