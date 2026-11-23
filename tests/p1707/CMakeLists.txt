set(p1707_sources "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1707/source")
add_test(NAME p1707_guarded_selector_native.integration COMMAND "${Python3_EXECUTABLE}" -B
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1707/run.py"
    "$<TARGET_FILE:newlangc>" "${p1707_sources}")
add_test(NAME p1707_guarded_selector_rollback.unit COMMAND p1673_probe
    guarded-selector-oom "${p1707_sources}/nested-7.nl" unused)
set_tests_properties(p1707_guarded_selector_rollback.unit PROPERTIES TIMEOUT 90)
