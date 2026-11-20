add_executable(r1573_public_probe tests/r1573/public_probe.c)
target_link_libraries(r1573_public_probe PRIVATE newlang_semantic
    newlang_parser)
add_test(NAME r1573_bytes.unit
    COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/tests/r1573/verify.py")
add_test(NAME r1573_public_p1.unit
    COMMAND "$<TARGET_FILE:r1573_public_probe>" P1
        "${CMAKE_SOURCE_DIR}/tests/r1573/sources/P1_zero_helper_unit_tail.nl")
add_test(NAME r1573_public_p2.unit
    COMMAND "$<TARGET_FILE:r1573_public_probe>" P2
        "${CMAKE_SOURCE_DIR}/tests/r1573/sources/P2_mixed_helper_final_unit.nl")
add_test(NAME r1573_cli_c17_native.integration
    COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/tests/r1573/run.py"
        "$<TARGET_FILE:newlangc>")
set_tests_properties(
    r1573_public_p1.unit r1573_public_p2.unit
    r1573_cli_c17_native.integration PROPERTIES DEPENDS r1573_bytes.unit)
