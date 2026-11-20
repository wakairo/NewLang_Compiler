if(NOT BUILD_TESTING)
    return()
endif()

set(R1562_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tests/r1562")
set(R1562_EVIDENCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/r1562-evidence")
file(MAKE_DIRECTORY "${R1562_EVIDENCE_DIR}")
add_executable(r1562_probe "${R1562_SOURCE_DIR}/probe.c")
target_link_libraries(r1562_probe PRIVATE newlang_semantic newlang_parser)

set(R1562_CONFIG "${CMAKE_CURRENT_BINARY_DIR}/r1562-config.json")
configure_file("${R1562_SOURCE_DIR}/config.json.in" "${R1562_CONFIG}" @ONLY)
add_custom_target(r1562_metadata ALL
    COMMAND "${Python3_EXECUTABLE}" "${R1562_SOURCE_DIR}/metadata.py"
        "${R1562_CONFIG}"
    DEPENDS r1562_probe
    VERBATIM)

add_test(NAME r1562.public.integration
    COMMAND "${Python3_EXECUTABLE}" "${R1562_SOURCE_DIR}/run.py"
        public "${R1562_CONFIG}")
add_test(NAME r1562.cli_native.integration
    COMMAND "${Python3_EXECUTABLE}" "${R1562_SOURCE_DIR}/run.py"
        native "${R1562_CONFIG}")
set_tests_properties(r1562.public.integration PROPERTIES
    FIXTURES_SETUP r1562_public)
set_tests_properties(r1562.cli_native.integration PROPERTIES
    FIXTURES_REQUIRED r1562_public)
set_tests_properties(r1562.public.integration r1562.cli_native.integration
    PROPERTIES TIMEOUT 180
    ENVIRONMENT "PYTHONDONTWRITEBYTECODE=1;PYTHONHASHSEED=0;LC_ALL=C;ASAN_OPTIONS=detect_leaks=1:halt_on_error=1;UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1")

file(APPEND "${CMAKE_CURRENT_BINARY_DIR}/CTestCustom.cmake"
    "\nlist(APPEND CTEST_CUSTOM_POST_TEST \"${Python3_EXECUTABLE} ${R1562_SOURCE_DIR}/transport.py ${R1562_EVIDENCE_DIR}\")\n")
message(STATUS "R1562 registered public and CLI/native tests; metadata target executes CTest0")
