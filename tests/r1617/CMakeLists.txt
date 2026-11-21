# R1617 own source preparation. Included once at the end of root CMakeLists.
if(NOT BUILD_TESTING)
    return()
endif()
if(TARGET r1617_observer)
    message(FATAL_ERROR "R1617 duplicate observer registration")
endif()
get_property(_r1617_before DIRECTORY PROPERTY TESTS)
if("r1617_multiarg.integration" IN_LIST _r1617_before)
    message(FATAL_ERROR "R1617 duplicate CTest registration")
endif()
add_executable(r1617_observer "${CMAKE_SOURCE_DIR}/tests/r1617/observer.c")
target_link_libraries(r1617_observer PRIVATE newlang_semantic newlang_parser)
set(_r1617_driver "${CMAKE_SOURCE_DIR}/tests/r1617/driver.py")
set(_r1617_raw "${CMAKE_BINARY_DIR}/r1617-raw")
add_test(NAME r1617_multiarg.integration
    COMMAND "${Python3_EXECUTABLE}" "${_r1617_driver}"
        --compiler "$<TARGET_FILE:newlangc>"
        --observer "$<TARGET_FILE:r1617_observer>"
        --cc "${CMAKE_C_COMPILER}"
        --sanitizer "${NEWLANG_SANITIZER}"
        --build-type "${CMAKE_BUILD_TYPE}"
        --source-dir "${CMAKE_SOURCE_DIR}/tests/r1617"
        --raw-dir "${_r1617_raw}")
set_tests_properties(r1617_multiarg.integration PROPERTIES
    RUN_SERIAL TRUE
    TIMEOUT 420
    ENVIRONMENT "PYTHONDONTWRITEBYTECODE=1;PYTHONHASHSEED=0;LC_ALL=C;ASAN_OPTIONS=detect_leaks=1:halt_on_error=1;UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1")
file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/r1617-expected-command.txt"
    CONTENT "${Python3_EXECUTABLE}\n${_r1617_driver}\n--compiler\n$<TARGET_FILE:newlangc>\n--observer\n$<TARGET_FILE:r1617_observer>\n--cc\n${CMAKE_C_COMPILER}\n--sanitizer\n${NEWLANG_SANITIZER}\n--build-type\n${CMAKE_BUILD_TYPE}\n--source-dir\n${CMAKE_SOURCE_DIR}/tests/r1617\n--raw-dir\n${_r1617_raw}\n")
