add_executable(r1551_public_probe tests/r1551/public_probe.c)
target_link_libraries(r1551_public_probe PRIVATE newlang_semantic)
set(r1551_data "${CMAKE_CURRENT_SOURCE_DIR}/tests/r1551")
set(r1551_work "${CMAKE_CURRENT_BINARY_DIR}/r1551-evidence")
add_test(NAME r1551_source_owned_negative.unit
    COMMAND "${Python3_EXECUTABLE}" "${r1551_data}/runner.py" source
        "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:r1551_public_probe>"
        "${r1551_data}" "${r1551_work}"
        "${NEWLANG_SANITIZER}" "${NEWLANG_FRONTEND_ONLY}")
add_test(NAME r1551_tail_c17_native.integration
    COMMAND "${Python3_EXECUTABLE}" "${r1551_data}/runner.py" tail
        "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:r1551_public_probe>"
        "${r1551_data}" "${r1551_work}"
        "${NEWLANG_SANITIZER}" "${NEWLANG_FRONTEND_ONLY}")
add_test(NAME r1551_return_c17_native.integration
    COMMAND "${Python3_EXECUTABLE}" "${r1551_data}/runner.py" return
        "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:r1551_public_probe>"
        "${r1551_data}" "${r1551_work}"
        "${NEWLANG_SANITIZER}" "${NEWLANG_FRONTEND_ONLY}")
set_tests_properties(r1551_source_owned_negative.unit
    PROPERTIES FIXTURES_SETUP r1551_source_owned_gate)
set_tests_properties(r1551_tail_c17_native.integration
    r1551_return_c17_native.integration
    PROPERTIES FIXTURES_REQUIRED r1551_source_owned_gate)
get_property(r1551_registered DIRECTORY PROPERTY TESTS)
list(FILTER r1551_registered INCLUDE REGEX "^r1551_")
list(LENGTH r1551_registered r1551_registered_count)
if(NOT r1551_registered_count EQUAL 3)
    message(FATAL_ERROR "R1551 must register exactly three unique CTests")
endif()
foreach(r1551_test IN LISTS r1551_registered)
    message(STATUS "R1551 registered CTest: ${r1551_test}")
endforeach()
add_custom_target(r1551_registration_inventory ALL
    COMMAND "${CMAKE_CTEST_COMMAND}" --test-dir "${CMAKE_CURRENT_BINARY_DIR}"
        -N -R "^r1551_"
    VERBATIM)
