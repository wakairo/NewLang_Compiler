# Independent R1395 source fixtures; only this NEVERMERGE R branch.
set(r1395_dir "${CMAKE_CURRENT_SOURCE_DIR}/tests/r1395")
set(r1395_p0 "${r1395_dir}/sources/P0.nl")
set(r1395_n1 "${r1395_dir}/sources/N1.nl")
set(r1395_n2 "${r1395_dir}/sources/N2.nl")
set(r1395_runner "${r1395_dir}/runner.py")

add_executable(r1395_owned_probe "${r1395_dir}/owned_probe.c")
target_link_libraries(r1395_owned_probe PRIVATE newlang_semantic newlang_parser
    newlang_checked_c_node)
# Force the native observer's C17 compilation even during R-FAST host build.
add_library(r1395_observer STATIC "${r1395_dir}/observer.c")
target_link_libraries(r1395_observer PRIVATE newlang_options)

add_test(NAME r1395_p0_check.integration COMMAND "${Python3_EXECUTABLE}"
    "${r1395_runner}" positive "$<TARGET_FILE:newlangc>"
    "${r1395_p0}" P0 "${NEWLANG_SANITIZER}" "${r1395_dir}/observer.c")
add_test(NAME r1395_p0_owned.unit COMMAND "$<TARGET_FILE:r1395_owned_probe>"
    "${r1395_p0}")
add_test(NAME r1395_p0_native.integration COMMAND "${Python3_EXECUTABLE}"
    "${r1395_runner}" native "$<TARGET_FILE:newlangc>"
    "${r1395_p0}" "${CMAKE_C_COMPILER}" "${NEWLANG_SANITIZER}"
    "${r1395_dir}/observer.c")
add_test(NAME r1395_n1_reject.integration COMMAND "${Python3_EXECUTABLE}"
    "${r1395_runner}" negative "$<TARGET_FILE:newlangc>"
    "${r1395_n1}" N1 "${NEWLANG_SANITIZER}" "${r1395_dir}/observer.c")
add_test(NAME r1395_n2_reject.integration COMMAND "${Python3_EXECUTABLE}"
    "${r1395_runner}" negative "$<TARGET_FILE:newlangc>"
    "${r1395_n2}" N2 "${NEWLANG_SANITIZER}" "${r1395_dir}/observer.c")
set_tests_properties(
    r1395_p0_check.integration
    r1395_p0_owned.unit
    r1395_p0_native.integration
    r1395_n1_reject.integration
    r1395_n2_reject.integration
    PROPERTIES TIMEOUT 90 RUN_SERIAL TRUE
    ENVIRONMENT "PYTHONDONTWRITEBYTECODE=1;PYTHONHASHSEED=0;LC_ALL=C;ASAN_OPTIONS=detect_leaks=1:halt_on_error=1;UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1"
)
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set_tests_properties(r1395_p0_native.integration PROPERTIES
        ENVIRONMENT "R1395_RELEASE=1;PYTHONDONTWRITEBYTECODE=1;PYTHONHASHSEED=0;LC_ALL=C;ASAN_OPTIONS=detect_leaks=1:halt_on_error=1;UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1")
endif()
