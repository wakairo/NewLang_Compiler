# Independent R1510 FIRST V1; FAST ctest -N registration only.
add_test(NAME r1511_first_v1_integrity.unit
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/r1511/probe.py"
        integrity "$<TARGET_FILE:newlangc>" "${CMAKE_C_COMPILER}" "${NEWLANG_SANITIZER}")
add_test(NAME r1511_first_v1_cohort.integration
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/r1511/probe.py"
        cohort "$<TARGET_FILE:newlangc>" "${CMAKE_C_COMPILER}" "${NEWLANG_SANITIZER}")
set_tests_properties(r1511_first_v1_integrity.unit r1511_first_v1_cohort.integration
    PROPERTIES TIMEOUT 240)
