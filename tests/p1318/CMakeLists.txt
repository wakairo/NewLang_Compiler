add_executable(p1318_owned tests/p1318/owned.c)
add_executable(p1318_cli_oom tests/p1318/cli_oom.c)
foreach(target p1318_owned p1318_cli_oom)
    target_include_directories(${target} PRIVATE src)
    target_compile_options(${target} PRIVATE -UNDEBUG)
    target_link_libraries(${target} PRIVATE newlang_checked_c_node)
    target_link_options(${target} PRIVATE
        -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc)
endforeach()
target_link_options(p1318_cli_oom PRIVATE -Wl,--wrap=fflush)
add_test(NAME p1318_owned_current_oom.unit COMMAND p1318_owned
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1318/A-index0.nl"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1318/B-index1.nl")
add_test(NAME p1318_negative_oom.integration COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1318/negative.py"
    "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:p1318_cli_oom>"
    "${CMAKE_CURRENT_BINARY_DIR}/p1318-negative")
add_test(NAME p1318_native.integration COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1318/native.py" "$<TARGET_FILE:newlangc>"
    "${CMAKE_C_COMPILER}" "${CMAKE_CURRENT_BINARY_DIR}/p1318-native")
set_tests_properties(p1318_owned_current_oom.unit
    p1318_negative_oom.integration p1318_native.integration PROPERTIES TIMEOUT 180)
