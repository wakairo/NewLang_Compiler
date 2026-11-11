add_executable(p1326_owned tests/p1326/owned.c)
add_executable(p1326_cli_oom tests/p1326/cli_oom.c)
foreach(target p1326_owned p1326_cli_oom)
    target_include_directories(${target} PRIVATE src)
    target_compile_options(${target} PRIVATE -UNDEBUG)
    target_link_libraries(${target} PRIVATE newlang_checked_c_node)
    target_link_options(${target} PRIVATE
        -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc)
endforeach()
target_link_options(p1326_cli_oom PRIVATE -Wl,--wrap=fflush)
add_test(NAME p1326_owned_current_oom.unit COMMAND p1326_owned
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1326/A-helper.nl"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1326/N-paired-helper.nl")
add_test(NAME p1326_negative_oom.integration COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1326/negative.py"
    "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:p1326_cli_oom>"
    "${CMAKE_CURRENT_BINARY_DIR}/p1326-negative")
add_test(NAME p1326_native.integration COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1326/native.py"
    "$<TARGET_FILE:newlangc>" "${CMAKE_C_COMPILER}"
    "${CMAKE_CURRENT_BINARY_DIR}/p1326-native")
set_tests_properties(p1326_owned_current_oom.unit
    p1326_negative_oom.integration p1326_native.integration PROPERTIES TIMEOUT 180)
