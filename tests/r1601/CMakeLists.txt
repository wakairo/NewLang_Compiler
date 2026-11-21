include_guard(GLOBAL)

add_executable(r1601_observe "${CMAKE_CURRENT_LIST_DIR}/observe.c")
target_link_libraries(r1601_observe PRIVATE
    newlang_checked_c_node newlang_semantic newlang_parser)

add_test(NAME r1601_arithmetic_actuals.integration
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_LIST_DIR}/run.py"
        --newlangc "$<TARGET_FILE:newlangc>"
        --observer "$<TARGET_FILE:r1601_observe>"
        --cc "${CMAKE_C_COMPILER}"
        --compiler-id "${CMAKE_C_COMPILER_ID}"
        --sanitizer "${NEWLANG_SANITIZER}"
        "--build-type=${CMAKE_BUILD_TYPE}"
        --raw "${CMAKE_BINARY_DIR}/r1601-raw"
        --work "${CMAKE_BINARY_DIR}/r1601-work")
set_tests_properties(r1601_arithmetic_actuals.integration
    PROPERTIES TIMEOUT 420)

# Metadata only: prints the exact own-name CTest listing; runs zero tests.
add_custom_target(r1601_registration_metadata ALL
    COMMAND "${CMAKE_CTEST_COMMAND}" --test-dir "${CMAKE_BINARY_DIR}"
        -N -R "^r1601_arithmetic_actuals[.]integration$"
    DEPENDS r1601_observe newlangc
    VERBATIM)
