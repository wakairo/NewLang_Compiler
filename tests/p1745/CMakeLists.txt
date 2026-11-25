set(p1745_sources "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1745/source")
add_test(NAME p1745_direct_policy_writer_native.integration
    COMMAND "${Python3_EXECUTABLE}" -B
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1745/run.py" "$<TARGET_FILE:newlangc>"
    "${p1745_sources}")
add_test(NAME p1745_direct_policy_writer_rollback.unit COMMAND p1673_probe
    guarded-byte-policy-writer-oom "${p1745_sources}/variant-overflow-fallback.nl"
    unused)
set_tests_properties(p1745_direct_policy_writer_native.integration
    p1745_direct_policy_writer_rollback.unit PROPERTIES TIMEOUT 90)
