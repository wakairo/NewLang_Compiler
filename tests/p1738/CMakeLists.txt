set(p1738_sources "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1738/source")
add_test(NAME p1738_guarded_scoped_helper_native.integration
    COMMAND "${Python3_EXECUTABLE}" -B
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1738/run.py" "$<TARGET_FILE:newlangc>"
    "${p1738_sources}")
add_test(NAME p1738_guarded_scoped_helper_rollback.unit COMMAND p1673_probe
    guarded-byte-mode-helper-oom "${p1738_sources}/guarded-delegated-reversed.nl"
    unused)
set_tests_properties(p1738_guarded_scoped_helper_native.integration
    p1738_guarded_scoped_helper_rollback.unit PROPERTIES TIMEOUT 90)
