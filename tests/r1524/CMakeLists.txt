if(BUILD_TESTING)
    add_executable(r1524_public_probe
        "${CMAKE_CURRENT_LIST_DIR}/public_probe.c")
    target_link_libraries(r1524_public_probe PRIVATE newlang_checked_c_node)
    add_test(NAME r1524.first_packet
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_LIST_DIR}/run.py"
            "$<TARGET_FILE:newlangc>"
            "$<TARGET_FILE:r1524_public_probe>"
            "${CMAKE_CURRENT_LIST_DIR}"
            "${CMAKE_CURRENT_BINARY_DIR}/r1524-output"
            "${NEWLANG_SANITIZER}")
    set_tests_properties(r1524.first_packet PROPERTIES
        TIMEOUT 240
        ENVIRONMENT "PYTHONDONTWRITEBYTECODE=1;PYTHONHASHSEED=0;LC_ALL=C;ASAN_OPTIONS=detect_leaks=1:halt_on_error=1;UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1")
    if(NOT TEST r1524.first_packet)
        message(FATAL_ERROR "R1524 declared test was not registered")
    endif()
    get_property(r1524_all_tests DIRECTORY PROPERTY TESTS)
    set(r1524_matches 0)
    foreach(r1524_test IN LISTS r1524_all_tests)
        if(r1524_test STREQUAL "r1524.first_packet")
            math(EXPR r1524_matches "${r1524_matches} + 1")
        endif()
    endforeach()
    if(NOT r1524_matches EQUAL 1)
        message(FATAL_ERROR "R1524 test registration is not unique")
    endif()
    message(STATUS "R1524 REGISTERED r1524.first_packet")
    message(STATUS "R1524 configure/build: no semantic or native tests executed")
endif()
