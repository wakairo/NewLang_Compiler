foreach(target owned oom)
    add_executable(p1272_${target} tests/p1272/${target}.c)
    target_include_directories(p1272_${target} PRIVATE src)
    target_compile_options(p1272_${target} PRIVATE -UNDEBUG)
    target_link_libraries(p1272_${target} PRIVATE newlang_checked_c_node)
endforeach()
target_compile_options(p1272_oom PRIVATE -fno-pie)
target_link_options(p1272_oom PRIVATE -no-pie
    -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc)
add_test(NAME p1272_source_native.integration COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1272/source_native.py"
    "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:p1272_owned>"
    "${CMAKE_C_COMPILER}" "${NEWLANG_SANITIZER}"
    "${CMAKE_CURRENT_BINARY_DIR}/p1272-source-native")
set_tests_properties(p1272_source_native.integration PROPERTIES RUN_SERIAL TRUE)
add_test(NAME p1272_oom.unit COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1272/oom_gate.py"
    "$<TARGET_FILE:p1272_oom>" "${CMAKE_CURRENT_BINARY_DIR}/p1272-oom")
set_tests_properties(p1272_oom.unit PROPERTIES RUN_SERIAL TRUE)
cmake_language(DEFER CALL set_tests_properties p1272_source_native.integration PROPERTIES TIMEOUT 180)
cmake_language(DEFER CALL set_tests_properties p1272_oom.unit PROPERTIES TIMEOUT 300)
