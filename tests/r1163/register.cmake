# R1163-only registration. Existing assertions run from a byte-verified fixed-P
# tree where the two historical root-CMake digests remain meaningful.
if(BUILD_TESTING)
    set(R1163_FIXED_SOURCE "${CMAKE_BINARY_DIR}/r1163-fixed-p")
    function(add_test)
        set(r1163_args ${ARGV})
        if(ARGV0 STREQUAL "NAME" AND
           (ARGV1 STREQUAL "p965_freeze.unit" OR ARGV1 STREQUAL "p982_freeze.unit"))
            set(r1163_relocated)
            foreach(r1163_arg IN LISTS r1163_args)
                if(r1163_arg MATCHES "^${CMAKE_SOURCE_DIR}/tests/p(965|982)/freeze_test.py$")
                    string(REPLACE "${CMAKE_SOURCE_DIR}/" "${R1163_FIXED_SOURCE}/"
                        r1163_arg "${r1163_arg}")
                endif()
                list(APPEND r1163_relocated "${r1163_arg}")
            endforeach()
            _add_test(${r1163_relocated})
        else()
            _add_test(${r1163_args})
        endif()
    endfunction()
    function(r1163_register)
        find_package(Python3 REQUIRED COMPONENTS Interpreter)
        execute_process(COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_SOURCE_DIR}/tests/r1163/snapshot.py"
            "${CMAKE_SOURCE_DIR}" "${R1163_FIXED_SOURCE}"
            COMMAND_ERROR_IS_FATAL ANY)
        add_executable(r1163_public "${CMAKE_SOURCE_DIR}/tests/r1163/probe.c")
        target_link_libraries(r1163_public PRIVATE newlang_semantic newlang_parser)
        _add_test(NAME r1163.runtime_return.integration
            COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tests/r1163/driver.py"
            "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:r1163_public>"
            "${CMAKE_C_COMPILER}" "${NEWLANG_SANITIZER}"
            "${CMAKE_SOURCE_DIR}/tests/r1163" "${CMAKE_BINARY_DIR}/r1163-evidence")
        set_tests_properties(r1163.runtime_return.integration PROPERTIES
            TIMEOUT 600 RUN_SERIAL TRUE)
    endfunction()
    cmake_language(DEFER CALL r1163_register)
endif()
