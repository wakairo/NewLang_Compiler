foreach(target owned oom)
    add_executable(p1248_${target} tests/p1248/${target}.c)
    target_include_directories(p1248_${target} PRIVATE src)
    target_compile_options(p1248_${target} PRIVATE -UNDEBUG)
    target_link_libraries(p1248_${target} PRIVATE newlang_checked_c_node)
endforeach()
target_compile_definitions(p1248_owned PRIVATE P1248_EMIT_SAFETY)
target_compile_options(p1248_oom PRIVATE -fno-pie)
target_link_options(p1248_oom PRIVATE -no-pie
    -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc)
add_test(NAME p1248_source_native.integration COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1248/source_native.py"
    "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:p1248_owned>"
    "${CMAKE_C_COMPILER}" "${NEWLANG_SANITIZER}"
    "${CMAKE_CURRENT_BINARY_DIR}/p1248-source-native")
set_tests_properties(p1248_source_native.integration PROPERTIES RUN_SERIAL TRUE TIMEOUT 180)
add_test(NAME p1248_oom.unit COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1248/oom_gate.py"
    "$<TARGET_FILE:p1248_oom>" "${CMAKE_CURRENT_BINARY_DIR}/p1248-oom")
set_tests_properties(p1248_oom.unit PROPERTIES RUN_SERIAL TRUE TIMEOUT 300)
