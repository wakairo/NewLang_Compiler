# R1185-owned enrollment. Original commands use the fixed P Git context.
if(BUILD_TESTING)
    set(R1185_FIXED_SOURCE "${CMAKE_BINARY_DIR}/r1185-fixed-p")
    function(add_test)
        set(r1185_relocated)
        foreach(r1185_arg IN LISTS ARGV)
            string(FIND "${r1185_arg}" "${CMAKE_SOURCE_DIR}/"
                r1185_source_path)
            if(r1185_arg STREQUAL "${CMAKE_SOURCE_DIR}")
                set(r1185_arg "${R1185_FIXED_SOURCE}")
            elseif(NOT r1185_source_path EQUAL -1)
                string(REPLACE "${CMAKE_SOURCE_DIR}/" "${R1185_FIXED_SOURCE}/"
                    r1185_arg "${r1185_arg}")
            endif()
            list(APPEND r1185_relocated "${r1185_arg}")
        endforeach()
        _add_test(${r1185_relocated})
    endfunction()
    function(r1185_register)
        find_package(Python3 REQUIRED COMPONENTS Interpreter)
        execute_process(COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_SOURCE_DIR}/tests/r1185/snapshot.py"
            "${CMAKE_SOURCE_DIR}" "${R1185_FIXED_SOURCE}"
            COMMAND_ERROR_IS_FATAL ANY)
        add_executable(r1185_public
            "${CMAKE_SOURCE_DIR}/tests/r1185/public_probe.c")
        target_link_libraries(r1185_public PRIVATE
            newlang_semantic newlang_parser)
        _add_test(NAME r1185.helper_result_current.integration
            COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_SOURCE_DIR}/tests/r1185/driver.py"
            "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:r1185_public>"
            "${CMAKE_C_COMPILER}" "${NEWLANG_SANITIZER}"
            "${CMAKE_SOURCE_DIR}/tests/r1185"
            "${CMAKE_BINARY_DIR}/r1185-evidence"
            "${CMAKE_BINARY_DIR}/r1185-fixed-context.json")
        set_tests_properties(r1185.helper_result_current.integration PROPERTIES
            TIMEOUT 600 RUN_SERIAL TRUE
            ENVIRONMENT "PYTHONDONTWRITEBYTECODE=1;PYTHONHASHSEED=0;LC_ALL=C;ASAN_OPTIONS=detect_leaks=1:halt_on_error=1;UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1")
        get_property(r1185_registered DIRECTORY PROPERTY TESTS)
        list(FILTER r1185_registered INCLUDE
            REGEX "^r1185[.]helper_result_current[.]integration$")
        list(LENGTH r1185_registered r1185_registration_count)
        if(NOT r1185_registration_count EQUAL 1)
            message(FATAL_ERROR "R1185 actual test registration is not unique")
        endif()
        message(STATUS
            "R1185 ACTUAL_CTEST_REGISTRATION r1185.helper_result_current.integration count=${r1185_registration_count}")
        file(WRITE "${CMAKE_BINARY_DIR}/CTestCustom.cmake"
            "set(CTEST_CUSTOM_POST_TEST \"\\\"${Python3_EXECUTABLE}\\\" \\\"${CMAKE_SOURCE_DIR}/tests/r1185/export.py\\\" \\\"${CMAKE_BINARY_DIR}/r1185-evidence\\\"\")\n")
    endfunction()
    cmake_language(DEFER CALL r1185_register)
endif()
