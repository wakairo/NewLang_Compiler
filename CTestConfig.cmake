# R1129 C221c: new opt-in registration only; existing root CMake stays unchanged.
include_guard(GLOBAL)

function(r1129_register)
    if(NOT BUILD_TESTING)
        return()
    endif()
    if(NOT TARGET newlang_semantic OR NOT TARGET newlangc
            OR NOT Python3_EXECUTABLE)
        message(FATAL_ERROR "R1129 public build targets/Python interface unavailable")
    endif()
    if(TARGET r1129_owned OR TARGET r1129_registration_inventory)
        message(FATAL_ERROR "R1129 duplicate target registration")
    endif()
    set(R1129_HARNESS "${PROJECT_SOURCE_DIR}/evidence/r1129/harness")
    add_executable(r1129_owned "${R1129_HARNESS}/owned.c")
    target_link_libraries(r1129_owned PRIVATE newlang_semantic)
    add_test(NAME r1129_c221_stutter.integration
        COMMAND "${Python3_EXECUTABLE}" "${R1129_HARNESS}/run.py"
            --root "${PROJECT_SOURCE_DIR}"
            --output "${CMAKE_BINARY_DIR}/r1129-raw"
            --cli "$<TARGET_FILE:newlangc>"
            --owned "$<TARGET_FILE:r1129_owned>"
            --compiler "${CMAKE_C_COMPILER}"
            --compiler-id "${CMAKE_C_COMPILER_ID}"
            --sanitizer "${NEWLANG_SANITIZER}"
            --build-type "${CMAKE_BUILD_TYPE}"
            --frontend-only "${NEWLANG_FRONTEND_ONLY}"
            "--host-cflags=${CMAKE_C_FLAGS}")
    set_tests_properties(r1129_c221_stutter.integration PROPERTIES
        TIMEOUT 300 RUN_SERIAL TRUE LABELS "r1129;independent")
    get_property(r1129_tests DIRECTORY PROPERTY TESTS)
    list(FILTER r1129_tests INCLUDE REGEX "^r1129_")
    list(LENGTH r1129_tests r1129_count)
    if(NOT r1129_count EQUAL 1)
        message(FATAL_ERROR "R1129 requires exactly one unique CTest")
    endif()
    add_custom_target(r1129_registration_inventory ALL
        COMMAND "${CMAKE_CTEST_COMMAND}" --test-dir "${CMAKE_BINARY_DIR}"
            -N -R "^r1129_c221_stutter[.]integration$"
        DEPENDS r1129_owned
        COMMENT "R1129 actual CTest registration inventory; no test execution"
        VERBATIM)
    file(READ "${R1129_HARNESS}/CTestCustom.cmake.in" r1129_template)
    string(CONFIGURE "${r1129_template}" r1129_custom @ONLY)
    if(EXISTS "${CMAKE_BINARY_DIR}/CTestCustom.cmake")
        file(READ "${CMAKE_BINARY_DIR}/CTestCustom.cmake" r1129_existing)
        if(NOT r1129_existing STREQUAL r1129_custom)
            message(FATAL_ERROR "R1129 refuses to replace foreign CTestCustom.cmake")
        endif()
    else()
        file(WRITE "${CMAKE_BINARY_DIR}/CTestCustom.cmake" "${r1129_custom}")
    endif()
endfunction()

cmake_language(DEFER CALL r1129_register)
