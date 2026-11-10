add_executable(p1291_owned tests/p1291/owned.c)
target_include_directories(p1291_owned PRIVATE src)
target_compile_options(p1291_owned PRIVATE -UNDEBUG)
target_link_libraries(p1291_owned PRIVATE newlang_checked_c_node)
add_executable(p1291_emit_owned tests/p1291/owned.c)
target_include_directories(p1291_emit_owned PRIVATE src)
target_compile_definitions(p1291_emit_owned PRIVATE P1291_EMIT=1)
target_compile_options(p1291_emit_owned PRIVATE -UNDEBUG)
target_link_libraries(p1291_emit_owned PRIVATE newlang_checked_c_node)
add_executable(p1291_check_oom tests/p1291/check_oom.c)
target_include_directories(p1291_check_oom PRIVATE src)
target_compile_options(p1291_check_oom PRIVATE -UNDEBUG -fno-pie)
target_link_libraries(p1291_check_oom PRIVATE newlang_checked_c_node)
target_link_options(p1291_check_oom PRIVATE -no-pie
    -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc)
add_executable(p1291_oom tests/p1291/oom.c)
target_include_directories(p1291_oom PRIVATE src)
target_compile_options(p1291_oom PRIVATE -UNDEBUG -fno-pie)
target_link_libraries(p1291_oom PRIVATE newlang_checked_c_node)
target_link_options(p1291_oom PRIVATE -no-pie
    -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc)
add_test(NAME p1291_source_check.unit COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1291/check.py"
    "$<TARGET_FILE:newlangc>" "${CMAKE_CURRENT_BINARY_DIR}/p1291-check")
add_test(NAME p1291_owned_A.unit COMMAND p1291_owned
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1291/sources/A-original.nl" 53 173 197 0)
add_test(NAME p1291_owned_B.unit COMMAND p1291_owned
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1291/sources/B-index-value-control.nl" 61 131 181 1)
add_test(NAME p1291_check_oom_A.unit COMMAND p1291_check_oom
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1291/sources/A-original.nl" check)
add_test(NAME p1291_check_oom_B.unit COMMAND p1291_check_oom
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1291/sources/B-index-value-control.nl" check)
set_tests_properties(p1291_check_oom_A.unit p1291_check_oom_B.unit
    PROPERTIES RUN_SERIAL TRUE TIMEOUT 90)
add_test(NAME p1291_emit_owned_A.unit COMMAND p1291_emit_owned
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1291/sources/A-original.nl" 53 173 197 0)
add_test(NAME p1291_emit_owned_B.unit COMMAND p1291_emit_owned
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1291/sources/B-index-value-control.nl" 61 131 181 1)
add_test(NAME p1291_native.integration COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1291/native.py"
    "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:p1291_owned>"
    "${CMAKE_C_COMPILER}" "${NEWLANG_SANITIZER}" "${CMAKE_CURRENT_BINARY_DIR}/p1291-native")
set_tests_properties(p1291_native.integration PROPERTIES RUN_SERIAL TRUE TIMEOUT 180)
add_test(NAME p1291_negative_output.unit COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1291/failure.py" negative
    "$<TARGET_FILE:p1291_oom>" "$<TARGET_FILE:newlangc>" "${CMAKE_CURRENT_BINARY_DIR}/p1291-negative")
add_test(NAME p1291_api_oom.unit COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1291/failure.py" api
    "$<TARGET_FILE:p1291_oom>" "$<TARGET_FILE:newlangc>" "${CMAKE_CURRENT_BINARY_DIR}/p1291-api-oom")
add_test(NAME p1291_named_oom.integration COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1291/failure.py" named
    "$<TARGET_FILE:p1280_cli_oom>" "$<TARGET_FILE:newlangc>" "${CMAKE_CURRENT_BINARY_DIR}/p1291-named-oom")
set_tests_properties(p1291_api_oom.unit p1291_named_oom.integration
    PROPERTIES RUN_SERIAL TRUE)
cmake_language(DEFER CALL set_tests_properties p1291_native.integration
    PROPERTIES TIMEOUT 180)
cmake_language(DEFER CALL set_tests_properties p1291_api_oom.unit p1291_named_oom.integration
    PROPERTIES TIMEOUT 300)
