if(TARGET r1581_public_probe)
    return()
endif()

set(_r1581_dir "${CMAKE_CURRENT_LIST_DIR}")
add_executable(r1581_public_probe "${_r1581_dir}/public_probe.c")
target_link_libraries(r1581_public_probe PRIVATE newlang_semantic)

set(_r1581_args
    --probe "$<TARGET_FILE:r1581_public_probe>"
    --compiler "$<TARGET_FILE:newlangc>"
    --cc "${CMAKE_C_COMPILER}"
    --ctest "${CMAKE_CTEST_COMMAND}"
    --build "${CMAKE_BINARY_DIR}"
    --config "${CMAKE_BUILD_TYPE}"
    "--cflags=${CMAKE_C_FLAGS}"
    --sanitizer "${NEWLANG_SANITIZER}")
set(_r1581_driver "${_r1581_dir}/driver.py")

add_test(NAME r1581.public_positive
    COMMAND "${Python3_EXECUTABLE}" "${_r1581_driver}"
    --group public ${_r1581_args})
add_test(NAME r1581.true_invalid
    COMMAND "${Python3_EXECUTABLE}" "${_r1581_driver}"
    --group invalid ${_r1581_args})
add_test(NAME r1581.legal_coverage
    COMMAND "${Python3_EXECUTABLE}" "${_r1581_driver}"
    --group coverage ${_r1581_args})
add_test(NAME r1581.cli_c17_native
    COMMAND "${Python3_EXECUTABLE}" "${_r1581_driver}"
    --group native ${_r1581_args})
set_tests_properties(
    r1581.public_positive r1581.true_invalid r1581.legal_coverage
    PROPERTIES TIMEOUT 120)
set_tests_properties(r1581.cli_c17_native PROPERTIES TIMEOUT 300)
set_tests_properties(
    r1581.public_positive r1581.true_invalid r1581.legal_coverage
    r1581.cli_c17_native PROPERTIES ENVIRONMENT
    "PYTHONDONTWRITEBYTECODE=1;PYTHONHASHSEED=0;LC_ALL=C;ASAN_OPTIONS=detect_leaks=1:halt_on_error=1;UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1")

set(_r1581_inputs
    "${_r1581_dir}/frozen/r1581-p1.nl"
    "${_r1581_dir}/frozen/r1581-p2.nl"
    "${_r1581_dir}/frozen/r1581-n1-zero.nl"
    "${_r1581_dir}/frozen/r1581-n2-underflow.nl"
    "${_r1581_dir}/frozen/r1581-e1-named-divisor.nl"
    "${_r1581_dir}/frozen/r1581-e2-multiplication.nl"
    "${_r1581_dir}/frozen/r1581-oracle.txt"
    "${_r1581_dir}/frozen-manifest.json")
set(_r1581_stamp "${CMAKE_CURRENT_BINARY_DIR}/r1581-preflight.ok")
add_custom_command(
    OUTPUT "${_r1581_stamp}"
    COMMAND "${Python3_EXECUTABLE}" "${_r1581_driver}"
        --group preflight ${_r1581_args}
    COMMAND "${CMAKE_COMMAND}" -E touch "${_r1581_stamp}"
    DEPENDS r1581_public_probe newlangc
        "${_r1581_driver}" ${_r1581_inputs}
    COMMENT "R1581 authorized source/public preflight (not FAST CTest execution)"
    VERBATIM)
add_custom_target(r1581_apparatus_preflight ALL DEPENDS "${_r1581_stamp}")
