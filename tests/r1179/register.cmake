# R-owned enrollment; inherited commands retain fixed-P bytes and real index.
if(BUILD_TESTING)
    set(R1179_FIXED_SOURCE "${CMAKE_BINARY_DIR}/r1179-fixed-p")
    function(add_test)
        set(r1179_relocated)
        foreach(r1179_arg IN LISTS ARGV)
            string(FIND "${r1179_arg}" "${CMAKE_SOURCE_DIR}/tests/"
                r1179_source_prefix)
            if(r1179_arg STREQUAL "${CMAKE_SOURCE_DIR}")
                set(r1179_arg "${R1179_FIXED_SOURCE}")
            elseif(r1179_source_prefix EQUAL 0)
                string(REPLACE "${CMAKE_SOURCE_DIR}/" "${R1179_FIXED_SOURCE}/"
                    r1179_arg "${r1179_arg}")
            endif()
            list(APPEND r1179_relocated "${r1179_arg}")
        endforeach()
        _add_test(${r1179_relocated})
    endfunction()
    function(r1179_register)
        find_package(Python3 REQUIRED COMPONENTS Interpreter)
        execute_process(COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_SOURCE_DIR}/tests/r1179/snapshot.py"
            "${CMAKE_SOURCE_DIR}" "${R1179_FIXED_SOURCE}"
            COMMAND_ERROR_IS_FATAL ANY)
        add_executable(r1179_public "${CMAKE_SOURCE_DIR}/tests/r1179/probe.c")
        target_link_libraries(r1179_public PRIVATE newlang_semantic newlang_parser)
        _add_test(NAME r1179.phase_selected_current.integration
            COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tests/r1179/driver.py"
            "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:r1179_public>"
            "${CMAKE_C_COMPILER}" "${NEWLANG_SANITIZER}"
            "${CMAKE_SOURCE_DIR}/tests/r1179" "${CMAKE_BINARY_DIR}/r1179-evidence"
            "${R1179_FIXED_SOURCE}")
        set_tests_properties(r1179.phase_selected_current.integration PROPERTIES
            TIMEOUT 600 RUN_SERIAL TRUE)
        get_property(r1179_registered DIRECTORY PROPERTY TESTS)
        list(FILTER r1179_registered INCLUDE
            REGEX "^r1179[.]phase_selected_current[.]integration$")
        list(LENGTH r1179_registered r1179_registration_count)
        if(NOT r1179_registration_count EQUAL 1)
            message(FATAL_ERROR "R1179 actual test registration is not unique")
        endif()
        message(STATUS
            "R1179 ACTUAL_CTEST_REGISTRATION r1179.phase_selected_current.integration count=${r1179_registration_count}")
        # Export read-only evidence after CTest even when a passing test is quiet.
        # This is a log observer, never a semantic test or a workflow mutation.
        file(WRITE "${CMAKE_BINARY_DIR}/CTestCustom.cmake"
            "set(CTEST_CUSTOM_POST_TEST \"\\\"${Python3_EXECUTABLE}\\\" \\\"${CMAKE_SOURCE_DIR}/tests/r1179/export.py\\\" \\\"${CMAKE_BINARY_DIR}/r1179-evidence\\\"\")\n")
    endfunction()
    cmake_language(DEFER CALL r1179_register)
endif()
