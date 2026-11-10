add_executable(p1312_consumer tests/p1312/consumer_gate.c)
target_include_directories(p1312_consumer PRIVATE src)
target_compile_options(p1312_consumer PRIVATE -UNDEBUG)
target_link_libraries(p1312_consumer PRIVATE newlang_checked_c_node)
target_link_options(p1312_consumer PRIVATE
    -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc)
add_test(NAME p1312_owned_current_oom.unit COMMAND p1312_consumer positive
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1312/positive.nl")
add_test(NAME p1312_negative.integration COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1312/negative.py" "$<TARGET_FILE:newlangc>"
    "${CMAKE_CURRENT_BINARY_DIR}/p1312-negative")
add_test(NAME p1312_native.integration COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1312/native.py" "$<TARGET_FILE:newlangc>"
    "${CMAKE_C_COMPILER}" "${CMAKE_CURRENT_BINARY_DIR}/p1312-native")
set_tests_properties(p1312_owned_current_oom.unit PROPERTIES TIMEOUT 180)
set_tests_properties(p1312_negative.integration p1312_native.integration
    PROPERTIES TIMEOUT 180)
