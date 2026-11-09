add_executable(p1291_owned tests/p1291/owned.c)
target_include_directories(p1291_owned PRIVATE src)
target_compile_options(p1291_owned PRIVATE -UNDEBUG)
target_link_libraries(p1291_owned PRIVATE newlang_checked_c_node)
add_executable(p1291_check_oom tests/p1291/check_oom.c)
target_include_directories(p1291_check_oom PRIVATE src)
target_compile_options(p1291_check_oom PRIVATE -UNDEBUG -fno-pie)
target_link_libraries(p1291_check_oom PRIVATE newlang_checked_c_node)
target_link_options(p1291_check_oom PRIVATE -no-pie
    -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc)
add_test(NAME p1291_source_check.unit COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1291/check.py"
    "$<TARGET_FILE:newlangc>" "${CMAKE_CURRENT_BINARY_DIR}/p1291-check")
add_test(NAME p1291_owned_A.unit COMMAND p1291_owned
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1291/sources/A-original.nl" 53 173 197 0)
add_test(NAME p1291_owned_B.unit COMMAND p1291_owned
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1291/sources/B-index-value-control.nl" 61 131 181 1)
add_test(NAME p1291_check_oom_A.unit COMMAND p1291_check_oom
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1291/sources/A-original.nl" check)
add_test(NAME p1291_check_oom_B.unit COMMAND p1291_check_oom
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1291/sources/B-index-value-control.nl" check)
set_tests_properties(p1291_check_oom_A.unit p1291_check_oom_B.unit
    PROPERTIES RUN_SERIAL TRUE TIMEOUT 90)
