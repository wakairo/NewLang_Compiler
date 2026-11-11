add_executable(p1332_consumer tests/p1332/consumer.c)
add_executable(p1332_cli_oom tests/p1332/cli_oom.c)
foreach(target p1332_consumer p1332_cli_oom)
    target_include_directories(${target} PRIVATE src)
    target_compile_options(${target} PRIVATE -UNDEBUG)
    target_link_libraries(${target} PRIVATE newlang_checked_c_node)
    target_link_options(${target} PRIVATE
        -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc)
endforeach()
target_link_options(p1332_cli_oom PRIVATE -Wl,--wrap=fflush)
add_test(NAME p1332_callee_current_consumer.unit COMMAND p1332_consumer
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1332/A-callee-local.nl"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1332/B-index1.nl")
add_test(NAME p1332_negative_cli_oom.integration COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1332/negative.py" "$<TARGET_FILE:newlangc>"
    "$<TARGET_FILE:p1332_cli_oom>" "${CMAKE_CURRENT_BINARY_DIR}/p1332-negative")
add_test(NAME p1332_native.integration COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1332/native.py" "$<TARGET_FILE:newlangc>"
    "${CMAKE_C_COMPILER}" "${CMAKE_CURRENT_BINARY_DIR}/p1332-native")
set_tests_properties(p1332_callee_current_consumer.unit
    p1332_negative_cli_oom.integration p1332_native.integration PROPERTIES TIMEOUT 180)
