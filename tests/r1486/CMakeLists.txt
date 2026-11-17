# Independent R1486 / C1079 V1; only R-owned tests and frozen R1483 oracle.
# No production, issuer, workflow or source-law changes.
if(NOT BUILD_TESTING)
    return()
endif()
if(NOT TARGET newlangc OR NOT Python3_EXECUTABLE)
    message(FATAL_ERROR "R1486: original CLI or Python absent")
endif()
foreach(r1486_case IN ITEMS RZ RN RX)
    set(r1486_name "r1486_${r1486_case}_original_v1.integration")
    if(TEST ${r1486_name})
        message(FATAL_ERROR "R1486: duplicate CTest ${r1486_name}")
    endif()
    add_test(NAME ${r1486_name} COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/r1486/run_original_v1.py"
        "$<TARGET_FILE:newlangc>" "${CMAKE_C_COMPILER}" "${NEWLANG_SANITIZER}"
        "${r1486_case}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/r1486/sources/${r1486_case}.nl"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/r1486/oracle/ORACLE-V1.json")
    set_tests_properties(${r1486_name} PROPERTIES TIMEOUT 90
        ENVIRONMENT "PYTHONDONTWRITEBYTECODE=1;PYTHONHASHSEED=0;LC_ALL=C;ASAN_OPTIONS=detect_leaks=1:halt_on_error=1;UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1")
endforeach()
message(STATUS "R1486: exactly 3 independent V1 stage tests registered; not executed")
