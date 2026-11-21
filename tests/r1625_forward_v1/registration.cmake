if(NOT DEFINED R1625_FORWARD_CTEST OR NOT DEFINED R1625_FORWARD_BUILD)
    message(FATAL_ERROR "R1625_FORWARD registration arguments missing")
endif()
execute_process(
    COMMAND "${R1625_FORWARD_CTEST}" --test-dir "${R1625_FORWARD_BUILD}" -N
        -R "^r1625_forward_mixed_copy[.]integration$"
    RESULT_VARIABLE r1625_forward_status
    OUTPUT_VARIABLE r1625_forward_output
    ERROR_VARIABLE r1625_forward_error
    TIMEOUT 30)
message("${r1625_forward_output}")
if(NOT r1625_forward_status EQUAL 0 OR NOT r1625_forward_error STREQUAL "")
    message(FATAL_ERROR "R1625_FORWARD anchored CTest inventory failed: ${r1625_forward_error}")
endif()
string(REGEX MATCHALL
    "Test[ \t]+#[0-9]+:[ \t]+r1625_forward_mixed_copy[.]integration"
    r1625_forward_matches "${r1625_forward_output}")
list(LENGTH r1625_forward_matches r1625_forward_count)
if(NOT r1625_forward_count EQUAL 1 OR
        NOT r1625_forward_output MATCHES "Total Tests:[ \t]+1(\r?\n|$)")
    message(FATAL_ERROR "R1625_FORWARD exact registration is absent or duplicated")
endif()
# Additional metadata only: expose the generated effective own test properties.
execute_process(
    COMMAND "${R1625_FORWARD_CTEST}" --test-dir "${R1625_FORWARD_BUILD}" --show-only=json-v1
        -R "^r1625_forward_mixed_copy[.]integration$"
    RESULT_VARIABLE r1625_forward_json_status
    OUTPUT_VARIABLE r1625_forward_json_output
    ERROR_VARIABLE r1625_forward_json_error
    TIMEOUT 30)
message("R1625_FORWARD REGISTRATION JSON BEGIN")
message("${r1625_forward_json_output}")
message("R1625_FORWARD REGISTRATION JSON END")
if(NOT r1625_forward_json_status EQUAL 0 OR NOT r1625_forward_json_error STREQUAL "")
    message(FATAL_ERROR "R1625_FORWARD anchored CTest JSON failed: ${r1625_forward_json_error}")
endif()
# Check the effective CTest model before the build may hand off to CTest.
string(JSON r1625_forward_test_count LENGTH "${r1625_forward_json_output}" tests)
if(NOT r1625_forward_test_count EQUAL 1)
    message(FATAL_ERROR "R1625_FORWARD expected one test in effective JSON")
endif()
string(JSON r1625_forward_test_name GET "${r1625_forward_json_output}" tests 0 name)
if(NOT r1625_forward_test_name STREQUAL "r1625_forward_mixed_copy.integration")
    message(FATAL_ERROR "R1625_FORWARD unexpected effective test name")
endif()
string(JSON r1625_forward_properties GET "${r1625_forward_json_output}" tests 0 properties)
string(JSON r1625_forward_property_count LENGTH "${r1625_forward_properties}")
if(r1625_forward_property_count LESS 1)
    message(FATAL_ERROR "R1625_FORWARD effective test properties missing")
endif()
math(EXPR r1625_forward_last_property "${r1625_forward_property_count} - 1")
set(r1625_forward_timeout_count 0)
set(r1625_forward_environment_count 0)
foreach(r1625_forward_index RANGE ${r1625_forward_last_property})
    string(JSON r1625_forward_property_name GET "${r1625_forward_properties}" ${r1625_forward_index} name)
    if(r1625_forward_property_name STREQUAL "TIMEOUT")
        math(EXPR r1625_forward_timeout_count "${r1625_forward_timeout_count} + 1")
        string(JSON r1625_forward_timeout_type TYPE "${r1625_forward_properties}" ${r1625_forward_index} value)
        string(JSON r1625_forward_timeout GET "${r1625_forward_properties}" ${r1625_forward_index} value)
        if(NOT r1625_forward_timeout_type STREQUAL "NUMBER" OR NOT r1625_forward_timeout EQUAL 600)
            message(FATAL_ERROR "R1625_FORWARD effective TIMEOUT must be 600")
        endif()
    elseif(r1625_forward_property_name STREQUAL "ENVIRONMENT")
        math(EXPR r1625_forward_environment_count "${r1625_forward_environment_count} + 1")
        string(JSON r1625_forward_environment GET "${r1625_forward_properties}" ${r1625_forward_index} value)
        string(JSON r1625_forward_environment_type TYPE "${r1625_forward_properties}" ${r1625_forward_index} value)
        string(JSON r1625_forward_environment_length LENGTH "${r1625_forward_environment}")
        if(NOT r1625_forward_environment_type STREQUAL "ARRAY" OR NOT r1625_forward_environment_length EQUAL 5)
            message(FATAL_ERROR "R1625_FORWARD expected five effective ENV values")
        endif()
        set(r1625_forward_remaining_env
            "PYTHONDONTWRITEBYTECODE=1"
            "PYTHONHASHSEED=0"
            "LC_ALL=C"
            "ASAN_OPTIONS=detect_leaks=1:halt_on_error=1"
            "UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1")
        foreach(r1625_forward_env_index RANGE 0 4)
            string(JSON r1625_forward_env_type TYPE "${r1625_forward_environment}" ${r1625_forward_env_index})
            string(JSON r1625_forward_env GET "${r1625_forward_environment}" ${r1625_forward_env_index})
            list(FIND r1625_forward_remaining_env "${r1625_forward_env}" r1625_forward_env_member)
            if(NOT r1625_forward_env_type STREQUAL "STRING" OR r1625_forward_env_member LESS 0)
                message(FATAL_ERROR "R1625_FORWARD unexpected or duplicate effective ENV value")
            endif()
            list(REMOVE_ITEM r1625_forward_remaining_env "${r1625_forward_env}")
        endforeach()
    elseif(r1625_forward_property_name STREQUAL "ENVIRONMENT_MODIFICATION")
        message(FATAL_ERROR "R1625_FORWARD effective ENVIRONMENT_MODIFICATION is unexpected")
    endif()
endforeach()
if(NOT r1625_forward_timeout_count EQUAL 1 OR NOT r1625_forward_environment_count EQUAL 1)
    message(FATAL_ERROR "R1625_FORWARD effective TIMEOUT/ENV is missing or duplicated")
endif()
message("R1625_FORWARD EFFECTIVE REGISTRATION PASS: 1 exact CTest, TIMEOUT 600, 5 exact ENV")
message("R1625_FORWARD REGISTRATION ONLY: 1 exact CTest; semantic/native tests NOT RUN")
