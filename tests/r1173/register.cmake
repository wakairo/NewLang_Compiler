# R-owned enrollment; inherited commands retain fixed-P bytes and real index.
if(BUILD_TESTING)
    set(R1173_FIXED_SOURCE "${CMAKE_BINARY_DIR}/r1173-fixed-p")
    function(add_test)
        set(r1173_relocated)
        foreach(r1173_arg IN LISTS ARGV)
            string(FIND "${r1173_arg}" "${CMAKE_SOURCE_DIR}/tests/"
                r1173_source_prefix)
            if(r1173_source_prefix EQUAL 0)
                string(REPLACE "${CMAKE_SOURCE_DIR}/" "${R1173_FIXED_SOURCE}/"
                    r1173_arg "${r1173_arg}")
            endif()
            list(APPEND r1173_relocated "${r1173_arg}")
        endforeach()
        _add_test(${r1173_relocated})
    endfunction()
    function(r1173_register)
        find_package(Python3 REQUIRED COMPONENTS Interpreter)
        execute_process(COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_SOURCE_DIR}/tests/r1173/snapshot.py"
            "${CMAKE_SOURCE_DIR}" "${R1173_FIXED_SOURCE}"
            COMMAND_ERROR_IS_FATAL ANY)
        add_executable(r1173_public "${CMAKE_SOURCE_DIR}/tests/r1173/probe.c")
        target_link_libraries(r1173_public PRIVATE newlang_semantic newlang_parser)
        _add_test(NAME r1173.changing_current.integration
            COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tests/r1173/driver.py"
            "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:r1173_public>"
            "${CMAKE_C_COMPILER}" "${NEWLANG_SANITIZER}"
            "${CMAKE_SOURCE_DIR}/tests/r1173" "${CMAKE_BINARY_DIR}/r1173-evidence")
        set_tests_properties(r1173.changing_current.integration PROPERTIES
            TIMEOUT 600 RUN_SERIAL TRUE)
        get_property(r1173_registered DIRECTORY PROPERTY TESTS)
        list(FILTER r1173_registered INCLUDE
            REGEX "^r1173[.]changing_current[.]integration$")
        list(LENGTH r1173_registered r1173_registration_count)
        if(NOT r1173_registration_count EQUAL 1)
            message(FATAL_ERROR "R1173 actual test registration is not unique")
        endif()
        message(STATUS
            "R1173 ACTUAL_CTEST_REGISTRATION r1173.changing_current.integration count=${r1173_registration_count}")
        # Export read-only evidence after CTest even when a passing test is quiet.
        # This is a log observer, never a semantic test or a workflow mutation.
        file(WRITE "${CMAKE_BINARY_DIR}/CTestCustom.cmake"
            "set(CTEST_CUSTOM_POST_TEST \"\\\"${Python3_EXECUTABLE}\\\" \\\"${CMAKE_SOURCE_DIR}/tests/r1173/export.py\\\" \\\"${CMAKE_BINARY_DIR}/r1173-evidence\\\"\")\n")
    endfunction()
    cmake_language(DEFER CALL r1173_register)
endif()
