add_executable(p1299_owned tests/p1299/owned.c)
target_include_directories(p1299_owned PRIVATE src)
target_compile_options(p1299_owned PRIVATE -UNDEBUG)
target_link_libraries(p1299_owned PRIVATE newlang_checked_c_node)
add_executable(p1299_emit_owned tests/p1299/owned.c)
target_include_directories(p1299_emit_owned PRIVATE src)
target_compile_options(p1299_emit_owned PRIVATE -UNDEBUG)
target_compile_definitions(p1299_emit_owned PRIVATE P1299_EMIT=1)
target_link_libraries(p1299_emit_owned PRIVATE newlang_checked_c_node)
add_test(NAME p1299_owned.unit COMMAND p1299_owned
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1299/P-named-display.nl")
add_test(NAME p1299_emit_owned.unit COMMAND p1299_emit_owned
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1299/P-named-display.nl")
add_test(NAME p1299_native.integration COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1299/native.py"
    "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:p1299_owned>"
    "${CMAKE_C_COMPILER}" "${NEWLANG_SANITIZER}"
    "${CMAKE_CURRENT_BINARY_DIR}/p1299-native")
add_test(NAME p1299_negative_output.unit COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1299/failure.py" negative
    "$<TARGET_FILE:p1291_oom>" "$<TARGET_FILE:newlangc>"
    "${CMAKE_CURRENT_BINARY_DIR}/p1299-negative")
add_test(NAME p1299_check_oom.unit COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1299/failure.py" check
    "$<TARGET_FILE:p1291_check_oom>" "$<TARGET_FILE:newlangc>"
    "${CMAKE_CURRENT_BINARY_DIR}/p1299-check-oom")
add_test(NAME p1299_emit_oom.unit COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1299/failure.py" emit
    "$<TARGET_FILE:p1291_oom>" "$<TARGET_FILE:newlangc>"
    "${CMAKE_CURRENT_BINARY_DIR}/p1299-emit-oom")
add_test(NAME p1299_named_oom.integration COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1299/failure.py" named
    "$<TARGET_FILE:p1280_cli_oom>" "$<TARGET_FILE:newlangc>"
    "${CMAKE_CURRENT_BINARY_DIR}/p1299-named-oom")
add_test(NAME p1299_prior_C17.unit COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1299/compatibility.py"
    "$<TARGET_FILE:newlangc>" "${CMAKE_CURRENT_BINARY_DIR}/p1299-compatibility")
set_tests_properties(p1299_check_oom.unit p1299_emit_oom.unit
    p1299_named_oom.integration PROPERTIES RUN_SERIAL TRUE)
cmake_language(DEFER CALL set_tests_properties
    p1299_check_oom.unit p1299_emit_oom.unit p1299_named_oom.integration
    PROPERTIES TIMEOUT 300)
