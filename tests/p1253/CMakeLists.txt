foreach(target owned oom)
    add_executable(p1253_${target} tests/p1253/${target}.c)
    target_include_directories(p1253_${target} PRIVATE src)
    target_compile_options(p1253_${target} PRIVATE -UNDEBUG)
    target_link_libraries(p1253_${target} PRIVATE newlang_checked_c_node)
endforeach()
target_compile_definitions(p1253_owned PRIVATE P1253_EMIT_SAFETY)
target_compile_options(p1253_oom PRIVATE -fno-pie)
target_link_options(p1253_oom PRIVATE -no-pie
    -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc)
add_test(NAME p1253_source_native.integration COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1253/source_native.py"
    "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:p1253_owned>"
    "${CMAKE_C_COMPILER}" "${NEWLANG_SANITIZER}"
    "${CMAKE_CURRENT_BINARY_DIR}/p1253-source-native")
set_tests_properties(p1253_source_native.integration PROPERTIES RUN_SERIAL TRUE TIMEOUT 180)
add_test(NAME p1253_oom.unit COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1253/oom_gate.py"
    "$<TARGET_FILE:p1253_oom>" "${CMAKE_CURRENT_BINARY_DIR}/p1253-oom")
set_tests_properties(p1253_oom.unit PROPERTIES RUN_SERIAL TRUE TIMEOUT 300)

# Apply after the root default; preserve the full finite injection/native plan.
cmake_language(DEFER CALL set_tests_properties
    p1253_source_native.integration PROPERTIES TIMEOUT 180)
cmake_language(DEFER CALL set_tests_properties
    p1253_oom.unit PROPERTIES TIMEOUT 300)
