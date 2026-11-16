add_executable(p1471_probe tests/p1471/probe.c)
target_include_directories(p1471_probe PRIVATE src)
target_link_libraries(p1471_probe PRIVATE newlang_checked_c_node newlang_semantic newlang_parser)
target_link_options(p1471_probe PRIVATE -Wl,--wrap=malloc -Wl,--wrap=realloc -Wl,--wrap=nl_sem_clone)
set(p1471_base "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1471")
add_test(NAME p1471_terminal_owned_negative.unit COMMAND p1471_probe suite
    "${p1471_base}/P1468-B1-failure-return.nl"
    "${p1471_base}/P1468-N1-wrong-live-domain.nl")
add_test(NAME p1471_check_oom.unit COMMAND p1471_probe check-oom
    "${p1471_base}/P1468-B1-failure-return.nl")
foreach(point 1 2 3 4)
    add_test(NAME p1471_oom_${point}.unit COMMAND p1471_probe oom
        "${p1471_base}/P1468-B1-failure-return.nl" "${point}")
endforeach()
add_test(NAME p1471_cli_native.integration
    COMMAND "${Python3_EXECUTABLE}" "${p1471_base}/native.py"
    "$<TARGET_FILE:newlangc>" "${CMAKE_C_COMPILER}" "${p1471_base}"
    "${CMAKE_CURRENT_BINARY_DIR}/p1471-native" "${NEWLANG_SANITIZER}")
set_tests_properties(p1471_cli_native.integration PROPERTIES TIMEOUT 180)
add_test(NAME p1471_cli_native.integration COMMAND "${Python3_EXECUTABLE}"
    "${p1471_base}/native.py" "$<TARGET_FILE:newlangc>" "${CMAKE_C_COMPILER}"
    "${p1471_base}" "${CMAKE_CURRENT_BINARY_DIR}/p1471-native" "${NEWLANG_SANITIZER}")
set_tests_properties(p1471_cli_native.integration PROPERTIES TIMEOUT 180
    DEPENDS p1471_terminal_owned_negative.unit)
