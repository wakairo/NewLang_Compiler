set(p1682_sources "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1682/source")
add_executable(p1682_probe tests/p1682/probe.c)
target_link_libraries(p1682_probe PRIVATE newlang_checked_c_node newlang_semantic)
add_test(NAME p1682_result_custody.unit COMMAND p1682_probe
    "${p1682_sources}/repeated.nl")
add_test(NAME p1682_cli_native.integration COMMAND "${Python3_EXECUTABLE}" -B
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1682/run.py"
    "$<TARGET_FILE:newlangc>" "${CMAKE_C_COMPILER}" "${p1682_sources}")
foreach(case tail-let tail-discard)
    add_test(NAME p1682_${case}_rollback.unit COMMAND p1673_probe helper-oom
        "${p1682_sources}/${case}.nl" unused)
    set_tests_properties(p1682_${case}_rollback.unit PROPERTIES TIMEOUT 90)
endforeach()
