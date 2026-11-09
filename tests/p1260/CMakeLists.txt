foreach(target owned oom)
    add_executable(p1260_${target} tests/p1260/${target}.c)
    target_include_directories(p1260_${target} PRIVATE src)
    target_compile_options(p1260_${target} PRIVATE -UNDEBUG)
    target_link_libraries(p1260_${target} PRIVATE newlang_checked_c_node)
endforeach()
target_compile_options(p1260_oom PRIVATE -fno-pie)
target_link_options(p1260_oom PRIVATE -no-pie
    -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc)
add_test(NAME p1260_source_native.integration COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1260/source_native.py"
    "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:p1260_owned>"
    "${CMAKE_C_COMPILER}" "${NEWLANG_SANITIZER}"
    "${CMAKE_CURRENT_BINARY_DIR}/p1260-source-native")
set_tests_properties(p1260_source_native.integration PROPERTIES RUN_SERIAL TRUE TIMEOUT 180)
add_test(NAME p1260_oom.unit COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1260/oom_gate.py"
    "$<TARGET_FILE:p1260_oom>" "${CMAKE_CURRENT_BINARY_DIR}/p1260-oom")
set_tests_properties(p1260_oom.unit PROPERTIES RUN_SERIAL TRUE TIMEOUT 300)

# Apply after the root default; preserve the full finite injection/native plan.
cmake_language(DEFER CALL set_tests_properties
    p1260_source_native.integration PROPERTIES TIMEOUT 180)
cmake_language(DEFER CALL set_tests_properties
    p1260_oom.unit PROPERTIES TIMEOUT 300)
