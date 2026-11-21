if(NOT DEFINED R1608_CTEST OR NOT DEFINED R1608_BUILD)
    message(FATAL_ERROR "R1608 registration arguments missing")
endif()
execute_process(
    COMMAND "${R1608_CTEST}" --test-dir "${R1608_BUILD}" -N
        -R "^r1608_arm_quantity[.]integration$"
    RESULT_VARIABLE r1608_status
    OUTPUT_VARIABLE r1608_output
    ERROR_VARIABLE r1608_error
    TIMEOUT 30)
message("${r1608_output}")
if(NOT r1608_status EQUAL 0 OR NOT r1608_error STREQUAL "")
    message(FATAL_ERROR "R1608 anchored CTest inventory failed: ${r1608_error}")
endif()
string(REGEX MATCHALL
    "Test[ \t]+#[0-9]+:[ \t]+r1608_arm_quantity[.]integration"
    r1608_matches "${r1608_output}")
list(LENGTH r1608_matches r1608_count)
if(NOT r1608_count EQUAL 1 OR
        NOT r1608_output MATCHES "Total Tests:[ \t]+1(\r?\n|$)")
    message(FATAL_ERROR "R1608 exact registration is absent or duplicated")
endif()
# Additional metadata only: expose the generated effective own test properties.
execute_process(
    COMMAND "${R1608_CTEST}" --test-dir "${R1608_BUILD}" --show-only=json-v1
        -R "^r1608_arm_quantity[.]integration$"
    RESULT_VARIABLE r1608_json_status
    OUTPUT_VARIABLE r1608_json_output
    ERROR_VARIABLE r1608_json_error
    TIMEOUT 30)
message("R1608 REGISTRATION JSON BEGIN")
message("${r1608_json_output}")
message("R1608 REGISTRATION JSON END")
if(NOT r1608_json_status EQUAL 0 OR NOT r1608_json_error STREQUAL "")
    message(FATAL_ERROR "R1608 anchored CTest JSON failed: ${r1608_json_error}")
endif()
message("R1608 REGISTRATION ONLY: 1 exact CTest; semantic/native tests NOT RUN")
