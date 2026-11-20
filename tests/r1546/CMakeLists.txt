add_executable(r1546_public_probe tests/r1546/public_probe.c)
target_link_libraries(r1546_public_probe PRIVATE newlang_semantic newlang_parser)
set(r1546_sources "${CMAKE_SOURCE_DIR}/tests/r1546")
add_test(NAME r1546_v3_positive_public.unit
    COMMAND r1546_public_probe positive "${r1546_sources}/v3_positive.nl")
add_test(NAME r1546_v3_negative_public.unit
    COMMAND r1546_public_probe negative "${r1546_sources}/v3_negative.nl")
add_test(NAME r1546_v3_positive_cli.integration
    COMMAND "${Python3_EXECUTABLE}" "${r1546_sources}/native_observe.py"
        check-positive "$<TARGET_FILE:newlangc>"
        "${r1546_sources}/v3_positive.nl" "${CMAKE_C_COMPILER}"
        "${NEWLANG_SANITIZER}")
add_test(NAME r1546_v3_negative_cli.integration
    COMMAND "${Python3_EXECUTABLE}" "${r1546_sources}/native_observe.py"
        check-negative "$<TARGET_FILE:newlangc>"
        "${r1546_sources}/v3_negative.nl" "${CMAKE_C_COMPILER}"
        "${NEWLANG_SANITIZER}")
add_test(NAME r1546_v3_native_numeric.integration
    COMMAND "${Python3_EXECUTABLE}" "${r1546_sources}/native_observe.py"
        native "$<TARGET_FILE:newlangc>"
        "${r1546_sources}/v3_positive.nl" "${CMAKE_C_COMPILER}"
        "${NEWLANG_SANITIZER}")
set_tests_properties(r1546_v3_positive_public.unit
    r1546_v3_negative_public.unit r1546_v3_positive_cli.integration
    r1546_v3_negative_cli.integration r1546_v3_native_numeric.integration
    PROPERTIES TIMEOUT 120)
