set(p1730_sources "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1730/source")
add_test(NAME p1730_guarded_live_descriptor_native.integration
    COMMAND "${Python3_EXECUTABLE}" -B
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1730/run.py" "$<TARGET_FILE:newlangc>"
    "${p1730_sources}")
add_test(NAME p1730_guarded_live_descriptor_rollback.unit COMMAND p1673_probe
    guarded-byte-mode-oom "${p1730_sources}/early-reverse.nl" unused)
set_tests_properties(p1730_guarded_live_descriptor_native.integration
    p1730_guarded_live_descriptor_rollback.unit PROPERTIES TIMEOUT 90)
