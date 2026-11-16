add_executable(p1463_probe tests/p1463/probe.c)
target_include_directories(p1463_probe PRIVATE src)
target_link_libraries(p1463_probe PRIVATE newlang_checked_c_node newlang_semantic newlang_parser)
target_link_options(p1463_probe PRIVATE -Wl,--wrap=malloc -Wl,--wrap=realloc -Wl,--wrap=nl_sem_clone)
set(p1463_base "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1463")
add_test(NAME p1463_owned_current_auth_negative.unit
    COMMAND p1463_probe suite "${p1463_base}")
foreach(point 1 2 3 4)
    add_test(NAME p1463_oom_${point}.unit
        COMMAND p1463_probe "oom${point}" "${p1463_base}/A-post-cleanup.nl")
endforeach()
add_test(NAME p1463_cli_native.integration
    COMMAND "${Python3_EXECUTABLE}" "${p1463_base}/native.py"
        "$<TARGET_FILE:newlangc>" "${CMAKE_C_COMPILER}" "${p1463_base}"
        "${CMAKE_CURRENT_BINARY_DIR}/p1463-native" "${NEWLANG_SANITIZER}")
set_tests_properties(p1463_cli_native.integration PROPERTIES TIMEOUT 180)
