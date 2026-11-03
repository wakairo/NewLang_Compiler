cmake_minimum_required(VERSION 3.28)
project(NewLangR1109C205Sidecar LANGUAGES NONE)
enable_testing()
find_package(Python3 3.12 REQUIRED COMPONENTS Interpreter)

# This separate R-only sidecar never includes or alters the root CMakeLists.txt
# or any historical freeze manifest. It does not claim root CTest registration.
get_filename_component(_repo "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
set(_runner "${CMAKE_CURRENT_SOURCE_DIR}/runner.py")
if(NOT EXISTS "${_runner}" OR NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/observer.inc")
    message(FATAL_ERROR "R1109 sidecar runner/observer missing")
endif()
foreach(_required R1109_CLI R1109_GCC R1109_CLANG R1109_RAW_BASE)
    if(NOT DEFINED ${_required} OR "${${_required}}" STREQUAL "")
        message(FATAL_ERROR "R1109 missing required argument: ${_required}")
    endif()
endforeach()
if(NOT EXISTS "${R1109_CLI}")
    message(FATAL_ERROR "R1109 actual compiler binary missing")
endif()

function(r1109_case short_name mode filename digest which_case)
    set(name "r1109_c205_${short_name}.integration")
    if(TEST "${name}")
        message(FATAL_ERROR "R1109 duplicate sidecar CTest: ${name}")
    endif()
    if(NOT EXISTS "${_repo}/tests/r1109/sources/${filename}")
        message(FATAL_ERROR "R1109 source not found: ${filename}")
    endif()
    add_test(NAME "${name}"
        COMMAND "${Python3_EXECUTABLE}" "${_runner}"
        "${R1109_CLI}" "${mode}"
        "${_repo}/tests/r1109/sources/${filename}"
        "${digest}" "${which_case}" "${R1109_GCC}" "${R1109_CLANG}"
        "${R1109_RAW_BASE}")
    if("${mode}" STREQUAL "positive")
        set_tests_properties("${name}" PROPERTIES TIMEOUT 240)
    else()
        set_tests_properties("${name}" PROPERTIES TIMEOUT 45)
    endif()
    set_tests_properties("${name}" PROPERTIES
        ENVIRONMENT "PYTHONDONTWRITEBYTECODE=1;PYTHONHASHSEED=0;LC_ALL=C;ASAN_OPTIONS=detect_leaks=1:halt_on_error=1;UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1")
endfunction()

r1109_case(distinct_native positive v2_positive_distinct.nl
    7efdcb1179b2241ebe5c4990050a6a87321d04974fa34f68b8ad70ee74647707 1)
r1109_case(equal_bits_native positive v2_positive_equal_bits.nl
    34a1a5ae3ae37fa9af57db95c6d68d9c2e68ea4369d65466255951bc2fcba691 2)
r1109_case(unknown_field_negative negative v1_negative_unknown_field.nl
    bf0e3b16656ea677e5319aaecd073fb11295081df57b74d7edb1de69b699e850 0)
r1109_case(loan_escape_negative negative v1_negative_loan_escape.nl
    0e3c610baf59f4e94d76f0f26fc9c95a423972d183ba2abc02fcb121512b44e3 0)
r1109_case(nonlocal_break_negative negative v1_negative_nonlocal_break.nl
    4eb77187c62d89f0ba5fe2c03d24aa0277f098713a48442a9b2ebd880f0ba2ce 0)
r1109_case(wrong_argument_negative negative v1_negative_wrong_argument.nl
    b04b610d171b8b3a243e8e99634cea9b5171041855d62f06c0c4fcf7d4a962e0 0)

get_property(_names DIRECTORY PROPERTY TESTS)
list(LENGTH _names _count)
if(NOT _count EQUAL 6)
    message(FATAL_ERROR "R1109 expected exactly 6 sidecar CTests, got ${_count}")
endif()
foreach(_name IN LISTS _names)
    if(NOT _name MATCHES "^r1109_c205_[a-z0-9_]+\\.integration$")
        message(FATAL_ERROR "R1109 unapproved sidecar CTest ${_name}")
    endif()
endforeach()
message(STATUS "R1109 C205 sidecar registered exactly 6 R-only real CTests; root untouched")
