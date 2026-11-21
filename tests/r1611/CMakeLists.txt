include_guard(GLOBAL)

add_executable(r1611_observe "${CMAKE_CURRENT_LIST_DIR}/observe.c")
target_link_libraries(r1611_observe PRIVATE
    newlang_checked_c_node newlang_semantic newlang_parser)

add_test(NAME r1611_retained_actuals.integration
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_LIST_DIR}/run.py"
        --newlangc "$<TARGET_FILE:newlangc>"
        --observer "$<TARGET_FILE:r1611_observe>"
        --cc "${CMAKE_C_COMPILER}"
        --compiler-id "${CMAKE_C_COMPILER_ID}"
        --sanitizer "${NEWLANG_SANITIZER}"
        "--build-type=${CMAKE_BUILD_TYPE}"
        --raw "${CMAKE_BINARY_DIR}/r1611-raw"
        --work "${CMAKE_BINARY_DIR}/r1611-work")
set_tests_properties(r1611_retained_actuals.integration
    PROPERTIES TIMEOUT 420
    ENVIRONMENT "PYTHONDONTWRITEBYTECODE=1;PYTHONHASHSEED=0;LC_ALL=C;ASAN_OPTIONS=detect_leaks=1:halt_on_error=1;UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1")

# Metadata only: prints the exact own-name listing and generated properties.
# Both CTest invocations enumerate metadata and execute zero tests.
add_custom_target(r1611_registration_metadata ALL
    COMMAND "${CMAKE_CTEST_COMMAND}" --test-dir "${CMAKE_BINARY_DIR}"
        -N -R "^r1611_retained_actuals[.]integration$"
    COMMAND "${CMAKE_CTEST_COMMAND}" --test-dir "${CMAKE_BINARY_DIR}"
        --show-only=json-v1 -R "^r1611_retained_actuals[.]integration$"
    DEPENDS r1611_observe newlangc
    VERBATIM)
