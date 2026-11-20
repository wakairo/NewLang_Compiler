set(R1557_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/tests/r1557")
set(R1557_EVIDENCE "${CMAKE_CURRENT_BINARY_DIR}/r1557-evidence")
set(R1557_DRIVER "${R1557_ROOT}/driver.py")
find_program(R1557_GCC NAMES gcc REQUIRED)

add_executable(r1557_public_probe "${R1557_ROOT}/public_probe.c")
target_link_libraries(r1557_public_probe PRIVATE newlang_semantic)

add_test(NAME r1557_remainder_public_auth.unit
    COMMAND "${Python3_EXECUTABLE}" "${R1557_DRIVER}" public
    --root "${R1557_ROOT}" --evidence "${R1557_EVIDENCE}"
    --probe "$<TARGET_FILE:r1557_public_probe>")
set_tests_properties(r1557_remainder_public_auth.unit PROPERTIES
    FIXTURES_SETUP r1557_public_ready)

add_test(NAME r1557_remainder_cli_native.integration
    COMMAND "${Python3_EXECUTABLE}" "${R1557_DRIVER}" native
    --root "${R1557_ROOT}" --evidence "${R1557_EVIDENCE}"
    --compiler "$<TARGET_FILE:newlangc>" --gcc "${R1557_GCC}"
    --frontend-only "${NEWLANG_FRONTEND_ONLY}"
    --sanitizer "${NEWLANG_SANITIZER}")
set_tests_properties(r1557_remainder_cli_native.integration PROPERTIES
    FIXTURES_REQUIRED r1557_public_ready)

add_custom_target(r1557_registration_inventory ALL
    COMMAND "${Python3_EXECUTABLE}" "${R1557_DRIVER}" inventory
    --evidence "${R1557_EVIDENCE}" --ctest "${CMAKE_CTEST_COMMAND}"
    --build-dir "${CMAKE_CURRENT_BINARY_DIR}"
    DEPENDS r1557_public_probe newlangc VERBATIM)

set(_r1557_custom "${CMAKE_CURRENT_BINARY_DIR}/CTestCustom.cmake")
set(_r1557_existing "")
if(EXISTS "${_r1557_custom}")
    file(READ "${_r1557_custom}" _r1557_existing)
endif()
string(FIND "${_r1557_existing}" "# R1557 own evidence transport" _r1557_marker)
if(_r1557_marker EQUAL -1)
    file(APPEND "${_r1557_custom}"
        "\n# R1557 own evidence transport\n"
        "list(APPEND CTEST_CUSTOM_POST_TEST \"${Python3_EXECUTABLE} ${R1557_DRIVER} transport --evidence ${R1557_EVIDENCE}\")\n")
endif()
message(STATUS "R1557 registered exactly two tests; build inventory is CTest -N only")
