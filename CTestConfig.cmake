# R1134 C223: new independent registration; existing root CMake stays unchanged.
include_guard(GLOBAL)

function(r1134_register)
    if(NOT BUILD_TESTING)
        return()
    endif()
    if(NOT TARGET newlang_semantic OR NOT TARGET newlangc
            OR NOT Python3_EXECUTABLE)
        message(FATAL_ERROR "R1134 public build targets/Python unavailable")
    endif()
    if(TARGET r1134_public_probe OR TARGET r1134_registration_inventory)
        message(FATAL_ERROR "R1134 duplicate target registration")
    endif()
    set(R1134_HARNESS "${PROJECT_SOURCE_DIR}/tests/r1134")
    add_executable(r1134_public_probe "${R1134_HARNESS}/public_probe.c")
    target_link_libraries(r1134_public_probe PRIVATE newlang_semantic)
    add_test(NAME r1134_guarded_loop_native.integration
        COMMAND "${Python3_EXECUTABLE}" "${R1134_HARNESS}/runner.py"
            "$<TARGET_FILE:newlangc>"
            "$<TARGET_FILE:r1134_public_probe>"
            "${CMAKE_C_COMPILER}" "${NEWLANG_SANITIZER}"
            "${CMAKE_BINARY_DIR}/r1134-raw")
    set_tests_properties(r1134_guarded_loop_native.integration PROPERTIES
        TIMEOUT 300 RUN_SERIAL TRUE LABELS "r1134;independent"
        ENVIRONMENT "PYTHONDONTWRITEBYTECODE=1;PYTHONHASHSEED=0;LC_ALL=C;ASAN_OPTIONS=detect_leaks=1:halt_on_error=1;UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1")
    get_property(r1134_tests DIRECTORY PROPERTY TESTS)
    list(FILTER r1134_tests INCLUDE REGEX "^r1134_")
    list(LENGTH r1134_tests r1134_count)
    if(NOT r1134_count EQUAL 1)
        message(FATAL_ERROR "R1134 requires exactly one unique CTest")
    endif()
    add_custom_target(r1134_registration_inventory ALL
        COMMAND "${CMAKE_CTEST_COMMAND}" --test-dir "${CMAKE_BINARY_DIR}"
            -N -R "^r1134_guarded_loop_native[.]integration$"
        DEPENDS r1134_public_probe newlangc
        COMMENT "R1134 actual CTest registration inventory; no test execution"
        VERBATIM)
    file(READ "${R1134_HARNESS}/CTestCustom.cmake.in" r1134_template)
    string(CONFIGURE "${r1134_template}" r1134_custom @ONLY)
    if(EXISTS "${CMAKE_BINARY_DIR}/CTestCustom.cmake")
        file(READ "${CMAKE_BINARY_DIR}/CTestCustom.cmake" r1134_existing)
        if(NOT r1134_existing STREQUAL r1134_custom)
            message(FATAL_ERROR "R1134 refuses to replace foreign CTestCustom.cmake")
        endif()
    else()
        file(WRITE "${CMAKE_BINARY_DIR}/CTestCustom.cmake" "${r1134_custom}")
    endif()
endfunction()

cmake_language(DEFER CALL r1134_register)
