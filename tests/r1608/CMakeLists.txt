include_guard(GLOBAL)
if(NOT BUILD_TESTING)
    return()
endif()

# R1620 TEXT only. C must separately release and bind a real immutable target.
# A cache override is not a release. This literal is changed only under that GO.
set(R1608_TARGET_SHA "f03dd0a76d5e93fe417e12e529271777fa961d67")
if(NEWLANG_FRONTEND_ONLY)
    set(r1608_mode "partial")
else()
    set(r1608_mode "full")
endif()
string(TOUPPER "${CMAKE_BUILD_TYPE}" r1608_configuration)

add_executable(r1608_observer "${CMAKE_CURRENT_LIST_DIR}/observe.c")
target_link_libraries(r1608_observer PRIVATE newlang_semantic newlang_parser)

add_test(NAME r1608_arm_quantity.integration
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_LIST_DIR}/driver.py"
        "--observer=$<TARGET_FILE:r1608_observer>"
        "--newlangc=$<TARGET_FILE:newlangc>"
        "--cc=${CMAKE_C_COMPILER}"
        "--sanitizer=${NEWLANG_SANITIZER}"
        "--mode=${r1608_mode}"
        "--target=${R1608_TARGET_SHA}"
        "--cflags=${CMAKE_C_FLAGS}"
        "--config-cflags=${CMAKE_C_FLAGS_${r1608_configuration}}"
        "--ldflags=${CMAKE_EXE_LINKER_FLAGS}"
        "--config-ldflags=${CMAKE_EXE_LINKER_FLAGS_${r1608_configuration}}")
set_tests_properties(r1608_arm_quantity.integration PROPERTIES
    TIMEOUT 600
    ENVIRONMENT "PYTHONDONTWRITEBYTECODE=1;PYTHONHASHSEED=0;LC_ALL=C;ASAN_OPTIONS=detect_leaks=1:halt_on_error=1;UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1"
    LABELS "independent-r1608;bounded-observation"
    RUN_SERIAL TRUE)

# Registration only, including R-FAST: this never invokes driver.py or a test.
add_custom_target(r1608_registration ALL
    COMMAND "${CMAKE_COMMAND}"
        "-DR1608_CTEST=${CMAKE_CTEST_COMMAND}"
        "-DR1608_BUILD=${CMAKE_BINARY_DIR}"
        -P "${CMAKE_CURRENT_LIST_DIR}/registration.cmake"
    DEPENDS r1608_observer newlangc
    VERBATIM)
