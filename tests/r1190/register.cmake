# R1190-owned enrollment. Original commands use the fixed P Git context.
if(BUILD_TESTING)
    set(R1190_FIXED_SOURCE "${CMAKE_BINARY_DIR}/r1190-fixed-p")
    function(add_test)
        set(r1190_relocated)
        foreach(r1190_arg IN LISTS ARGV)
            # Runtime tools and build products exist in the actual checkout,
            # not in the genuine tracked-source/Git snapshot. Preserve their
            # original argv even when the SDK is beneath CMAKE_SOURCE_DIR.
            set(r1190_runtime_path FALSE)
            foreach(r1190_runtime_root IN ITEMS
                    "${CMAKE_BINARY_DIR}" "${CMAKE_SOURCE_DIR}/.deps")
                string(FIND "${r1190_arg}" "${r1190_runtime_root}/"
                    r1190_runtime_prefix)
                if(r1190_arg STREQUAL r1190_runtime_root
                        OR NOT r1190_runtime_prefix EQUAL -1)
                    set(r1190_runtime_path TRUE)
                endif()
            endforeach()
            if(r1190_runtime_path)
                list(APPEND r1190_relocated "${r1190_arg}")
                continue()
            endif()
            string(FIND "${r1190_arg}" "${CMAKE_SOURCE_DIR}/"
                r1190_source_path)
            if(r1190_arg STREQUAL "${CMAKE_SOURCE_DIR}")
                set(r1190_arg "${R1190_FIXED_SOURCE}")
            elseif(NOT r1190_source_path EQUAL -1)
                string(REPLACE "${CMAKE_SOURCE_DIR}/" "${R1190_FIXED_SOURCE}/"
                    r1190_arg "${r1190_arg}")
            endif()
            list(APPEND r1190_relocated "${r1190_arg}")
        endforeach()
        _add_test(${r1190_relocated})
    endfunction()
    function(r1190_register)
        find_package(Python3 REQUIRED COMPONENTS Interpreter)
        execute_process(COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_SOURCE_DIR}/tests/r1190/snapshot.py"
            "${CMAKE_SOURCE_DIR}" "${R1190_FIXED_SOURCE}"
            COMMAND_ERROR_IS_FATAL ANY)
        add_executable(r1190_public
            "${CMAKE_SOURCE_DIR}/tests/r1190/public_probe.c")
        target_link_libraries(r1190_public PRIVATE
            newlang_semantic newlang_parser)
        _add_test(NAME r1190.phase_usize_copy.integration
            COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_SOURCE_DIR}/tests/r1190/driver.py"
            "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:r1190_public>"
            "${CMAKE_C_COMPILER}" "${NEWLANG_SANITIZER}"
            "${CMAKE_SOURCE_DIR}/tests/r1190"
            "${CMAKE_BINARY_DIR}/r1190-evidence"
            "${CMAKE_BINARY_DIR}/r1190-fixed-context.json"
            "${CMAKE_BUILD_TYPE}")
        set_tests_properties(r1190.phase_usize_copy.integration PROPERTIES
            TIMEOUT 600 RUN_SERIAL TRUE
            ENVIRONMENT "PYTHONDONTWRITEBYTECODE=1;PYTHONHASHSEED=0;LC_ALL=C;ASAN_OPTIONS=detect_leaks=1:halt_on_error=1;UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1")
        get_property(r1190_registered DIRECTORY PROPERTY TESTS)
        list(FILTER r1190_registered INCLUDE
            REGEX "^r1190[.]phase_usize_copy[.]integration$")
        list(LENGTH r1190_registered r1190_registration_count)
        if(NOT r1190_registration_count EQUAL 1)
            message(FATAL_ERROR "R1190 actual test registration is not unique")
        endif()
        message(STATUS
            "R1190 ACTUAL_CTEST_REGISTRATION r1190.phase_usize_copy.integration count=${r1190_registration_count}")
        file(WRITE "${CMAKE_BINARY_DIR}/CTestCustom.cmake"
            "set(CTEST_CUSTOM_POST_TEST \"\\\"${Python3_EXECUTABLE}\\\" \\\"${CMAKE_SOURCE_DIR}/tests/r1190/export.py\\\" \\\"${CMAKE_BINARY_DIR}/r1190-evidence\\\"\")\n")
    endfunction()
    cmake_language(DEFER CALL r1190_register)
endif()
