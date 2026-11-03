# R1120 V1 checks the immutable C214 source/oracle in GitHub Actions only.
# A positive-source refusal remains an explicit first BLIND INCONCLUSIVE/RED.
if(NOT TARGET newlangc OR NOT Python3_EXECUTABLE)
    message(FATAL_ERROR "R1120: original compiler/Python unavailable")
endif()
if(TEST r1120_v1_original.integration)
    message(FATAL_ERROR "R1120: duplicate independent CTest registration")
endif()
add_test(NAME r1120_v1_original.integration
    COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/redteam/r1120/verify.py"
        --compiler "$<TARGET_FILE:newlangc>"
        --source-dir "${CMAKE_CURRENT_SOURCE_DIR}/tests/redteam/r1120"
        --output-dir "${CMAKE_CURRENT_BINARY_DIR}/r1120-raw"
        --cc "${CMAKE_C_COMPILER}"
        --sanitizer "${NEWLANG_SANITIZER}"
        --phase v1)
set_tests_properties(r1120_v1_original.integration PROPERTIES
    TIMEOUT 90
    LABELS "independent-r;R1120;BLIND;v1"
    ENVIRONMENT "PYTHONDONTWRITEBYTECODE=1;PYTHONHASHSEED=0;LC_ALL=C;ASAN_OPTIONS=detect_leaks=1:halt_on_error=1;UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1")
message(STATUS "R1120: r1120_v1_original.integration registered; not executed")
