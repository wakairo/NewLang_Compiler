add_executable(p1323_owned tests/p1323/owned.c)
add_executable(p1323_cli_oom tests/p1323/cli_oom.c)
foreach(target p1323_owned p1323_cli_oom)
    target_include_directories(${target} PRIVATE src)
    target_compile_options(${target} PRIVATE -UNDEBUG)
    target_link_libraries(${target} PRIVATE newlang_checked_c_node)
    target_link_options(${target} PRIVATE
        -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc)
endforeach()
target_link_options(p1323_cli_oom PRIVATE -Wl,--wrap=fflush)
add_test(NAME p1323_owned_current_oom.unit COMMAND p1323_owned
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1318/N-two-field-boundary.nl"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1323/B-reversed-distinct.nl"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1169/sources/03-multiple.nl"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1312/positive.nl"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1318/A-index0.nl")
add_test(NAME p1323_negative_oom.integration COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1323/negative.py"
    "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:p1323_cli_oom>"
    "${CMAKE_CURRENT_BINARY_DIR}/p1323-negative")
add_test(NAME p1323_native.integration COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1323/native.py" "$<TARGET_FILE:newlangc>"
    "${CMAKE_C_COMPILER}" "${CMAKE_CURRENT_BINARY_DIR}/p1323-native")
set_tests_properties(p1323_owned_current_oom.unit
    p1323_negative_oom.integration p1323_native.integration PROPERTIES TIMEOUT 180)
