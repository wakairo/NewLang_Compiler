set(p1694_source "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1694/source")
add_test(NAME p1694_cli_native.integration COMMAND "${Python3_EXECUTABLE}" -B
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1694/run.py"
    "$<TARGET_FILE:newlangc>" "${p1694_source}")
foreach(case tail-div tail-rem)
    add_test(NAME p1694_${case}_rollback.unit COMMAND p1673_probe
        composed-product-oom "${p1694_source}/${case}.nl" unused)
    set_tests_properties(p1694_${case}_rollback.unit PROPERTIES TIMEOUT 90)
endforeach()
