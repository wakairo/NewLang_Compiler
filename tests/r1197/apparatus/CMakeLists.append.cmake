
# R1197 independent evidence only; NEVERMERGE. Original targets remain intact.
if(BUILD_TESTING)
    add_executable(r1197_probe tests/r1197/apparatus/probe.c)
    target_link_libraries(r1197_probe PRIVATE newlang_semantic newlang_parser)
    add_test(NAME r1197_independent_return.integration
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/r1197/apparatus/run.py"
            --compiler-cli "$<TARGET_FILE:newlangc>"
            --probe "$<TARGET_FILE:r1197_probe>"
            --cohort-root "${CMAKE_CURRENT_SOURCE_DIR}/tests/r1197"
            --evidence-dir "${CMAKE_CURRENT_BINARY_DIR}/r1197-evidence"
            --host-compiler "${CMAKE_C_COMPILER}"
            --host-build-type "${CMAKE_BUILD_TYPE}"
            --host-sanitizer "${NEWLANG_SANITIZER}")
    set_tests_properties(r1197_independent_return.integration
        PROPERTIES TIMEOUT 300 LABELS "R1197;NEVERMERGE"
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
    add_custom_target(r1197_registration_inventory ALL
        COMMAND "${CMAKE_CTEST_COMMAND}" --test-dir "${CMAKE_CURRENT_BINARY_DIR}"
            -N -R "^r1197_independent_return[.]integration$"
        DEPENDS r1197_probe
        COMMENT "R1197 actual CTest registration only; NO semantic test execution")
endif()
