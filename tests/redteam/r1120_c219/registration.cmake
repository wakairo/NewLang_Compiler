# C219 V3 prospective NON-BLIND calibrated independent source measurement.
if(NOT TARGET newlangc OR NOT Python3_EXECUTABLE)
    message(FATAL_ERROR "R1120 C219: original compiler/Python unavailable")
endif()
if(TEST r1120_c219_v3.integration)
    message(FATAL_ERROR "R1120 C219: duplicate independent CTest registration")
endif()
add_test(NAME r1120_c219_v3.integration
    COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/tests/redteam/r1120_c219/verify.py"
        --compiler "$<TARGET_FILE:newlangc>"
        --source-dir "${CMAKE_SOURCE_DIR}/tests/redteam/r1120_c219"
        --output-dir "${CMAKE_BINARY_DIR}/r1120-c219-raw"
        --cc "${CMAKE_C_COMPILER}"
        --sanitizer "${NEWLANG_SANITIZER}"
        --phase v3)
set_tests_properties(r1120_c219_v3.integration PROPERTIES
    TIMEOUT 180
    LABELS "independent-r;R1120;C219;NON-BLIND;v3"
    ENVIRONMENT "PYTHONDONTWRITEBYTECODE=1;PYTHONHASHSEED=0;LC_ALL=C;ASAN_OPTIONS=detect_leaks=1:halt_on_error=1;UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1")
message(STATUS "R1120 C219: r1120_c219_v3.integration registered; not executed")

# Build-time inventory prints this R test record from actual ctest -N.
# It never runs the compiler, public observer, native guest, or tests.
add_custom_target(r1120_c219_registration_inventory ALL
    COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/tests/redteam/r1120_c219/inventory.py"
        "${CMAKE_CTEST_COMMAND}" "${CMAKE_BINARY_DIR}"
    COMMENT "R1120 C219 actual CTest registration inventory only"
    VERBATIM)
add_dependencies(r1120_c219_registration_inventory newlangc)

# Standard CTest post-test raw transport; no frozen workflow changes.
configure_file(
    "${CMAKE_SOURCE_DIR}/tests/redteam/r1120_c219/CTestCustom.cmake.in"
    "${CMAKE_BINARY_DIR}/CTestCustom.cmake"
    @ONLY)
