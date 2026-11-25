set(p1745_sources "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1745/source")
add_test(NAME p1745_direct_policy_writer_native.integration
    COMMAND "${Python3_EXECUTABLE}" -B
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1745/run.py" "$<TARGET_FILE:newlangc>"
    "${p1745_sources}")
add_test(NAME p1745_direct_policy_writer_rollback.unit COMMAND p1673_probe
    guarded-byte-policy-writer-oom "${p1745_sources}/variant-overflow-fallback.nl"
    unused)
add_test(NAME p1748_named_policy_writer_rollback.unit COMMAND p1673_probe
    guarded-byte-named-policy-writer-oom
    "${p1745_sources}/named-variant-overflow-fallback.nl"
    unused)
add_test(NAME p1749_scoped_writer_unit_rollback.unit COMMAND p1673_probe
    guarded-byte-policy-writer-oom "${p1745_sources}/unit-tail.nl" unused)
add_test(NAME p1749_named_writer_unit_rollback.unit COMMAND p1673_probe
    guarded-byte-named-policy-writer-oom
    "${p1745_sources}/named-unit-tail.nl" unused)
add_test(NAME p1756_delegated_policy_rollback.unit COMMAND p1673_probe
    guarded-byte-delegated-policy-writer-oom
    "${p1745_sources}/delegated-unit-tail.nl" unused)
set_tests_properties(p1745_direct_policy_writer_native.integration
    p1745_direct_policy_writer_rollback.unit
    p1748_named_policy_writer_rollback.unit
    p1749_scoped_writer_unit_rollback.unit
    p1749_named_writer_unit_rollback.unit
    p1756_delegated_policy_rollback.unit PROPERTIES TIMEOUT 90)
