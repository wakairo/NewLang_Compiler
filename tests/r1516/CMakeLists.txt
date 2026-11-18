# R1516 calibrated independent R-owned V2. FAST only enumerates; FULL executes.
add_test(NAME r1516_v2_original_integrity.unit
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/r1516/probe.py"
        integrity "$<TARGET_FILE:newlangc>" "${CMAKE_C_COMPILER}" "${NEWLANG_SANITIZER}")
add_test(NAME r1516_v2_native_and_negative.integration
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/r1516/probe.py"
        cohort "$<TARGET_FILE:newlangc>" "${CMAKE_C_COMPILER}" "${NEWLANG_SANITIZER}")
set_tests_properties(r1516_v2_original_integrity.unit
    r1516_v2_native_and_negative.integration PROPERTIES TIMEOUT 300)
