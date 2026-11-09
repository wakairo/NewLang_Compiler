foreach(target owned oom cli_oom)
    add_executable(p1280_${target} tests/p1280/${target}.c)
    target_include_directories(p1280_${target} PRIVATE src)
    target_compile_options(p1280_${target} PRIVATE -UNDEBUG)
    target_link_libraries(p1280_${target} PRIVATE newlang_checked_c_node)
endforeach()
target_compile_options(p1280_oom PRIVATE -fno-pie)
target_link_options(p1280_oom PRIVATE -no-pie -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc)
target_link_options(p1280_cli_oom PRIVATE -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc
    -Wl,--wrap=nl_semantic_check_expression -Wl,--wrap=nl_checked_c_node)
add_test(NAME p1280_source_native.integration COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1280/source_native.py"
    "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:p1280_owned>"
    "${CMAKE_C_COMPILER}" "${NEWLANG_SANITIZER}" "${CMAKE_CURRENT_BINARY_DIR}/p1280-native")
add_test(NAME p1280_oom.unit COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1280/oom_gate.py"
    "$<TARGET_FILE:p1280_oom>" "${CMAKE_CURRENT_BINARY_DIR}/p1280-oom")
add_test(NAME p1280_named_oom.integration COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1280/named_oom.py"
    "$<TARGET_FILE:p1280_cli_oom>" "$<TARGET_FILE:newlangc>" "${CMAKE_CURRENT_BINARY_DIR}/p1280-named-oom")
set_tests_properties(p1280_source_native.integration p1280_oom.unit p1280_named_oom.integration PROPERTIES RUN_SERIAL TRUE)
cmake_language(DEFER CALL set_tests_properties p1280_source_native.integration PROPERTIES TIMEOUT 180)
cmake_language(DEFER CALL set_tests_properties p1280_oom.unit p1280_named_oom.integration PROPERTIES TIMEOUT 300)
