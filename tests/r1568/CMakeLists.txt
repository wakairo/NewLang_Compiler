if(NOT BUILD_TESTING)
    return()
endif()

add_executable(r1568_public tests/r1568/public.c)
target_link_libraries(r1568_public PRIVATE newlang_checked_c_node)

set(r1568_root "${CMAKE_CURRENT_LIST_DIR}")
set(r1568_args
    --cli "$<TARGET_FILE:newlangc>"
    --probe "$<TARGET_FILE:r1568_public>"
    --fixtures "${r1568_root}"
    --output "${CMAKE_BINARY_DIR}/r1568-evidence"
    --build-dir "${CMAKE_BINARY_DIR}"
    --host-cc "${CMAKE_C_COMPILER}"
    --sanitizer "${NEWLANG_SANITIZER}"
    --build-type "${CMAKE_BUILD_TYPE}"
    --frontend "${NEWLANG_FRONTEND_ONLY}")

add_test(NAME r1568.public.unit
    COMMAND "${Python3_EXECUTABLE}" "${r1568_root}/run.py" public ${r1568_args})
add_test(NAME r1568.cli_negative_control.integration
    COMMAND "${Python3_EXECUTABLE}" "${r1568_root}/run.py" controls ${r1568_args})
add_test(NAME r1568.c17_native.integration
    COMMAND "${Python3_EXECUTABLE}" "${r1568_root}/run.py" native ${r1568_args})

set_tests_properties(r1568.public.unit PROPERTIES
    FIXTURES_SETUP r1568_public_ready TIMEOUT 240 LABELS "independent-r;r1568")
set_tests_properties(r1568.cli_negative_control.integration PROPERTIES
    FIXTURES_REQUIRED r1568_public_ready FIXTURES_SETUP r1568_controls_ready
    TIMEOUT 240 LABELS "independent-r;r1568")
set_tests_properties(r1568.c17_native.integration PROPERTIES
    FIXTURES_REQUIRED "r1568_public_ready;r1568_controls_ready"
    TIMEOUT 240 LABELS "independent-r;r1568")

# Inventory only: retain the own actual CTest names in FAST build logs.
add_custom_target(r1568_registration ALL
    COMMAND "${CMAKE_CTEST_COMMAND}" --test-dir "${CMAKE_BINARY_DIR}"
        -N -R "^r1568[.]"
    DEPENDS r1568_public newlangc
    VERBATIM)
