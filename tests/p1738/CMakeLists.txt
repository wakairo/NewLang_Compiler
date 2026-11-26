set(p1738_sources "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1738/source")
add_test(NAME p1738_guarded_scoped_helper_native.integration
    COMMAND "${Python3_EXECUTABLE}" -B
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1738/run.py" "$<TARGET_FILE:newlangc>"
    "${p1738_sources}")
add_test(NAME p1738_guarded_scoped_helper_rollback.unit COMMAND p1673_probe
    guarded-byte-mode-helper-oom "${p1738_sources}/guarded-delegated-reversed.nl"
    unused)
add_test(NAME p1764_caller_selected_writer_rollback.unit COMMAND p1673_probe
    guarded-byte-mode-helper-oom "${p1738_sources}/split-queue-sibling-first.nl"
    unused)
add_test(NAME p1769_direct_displaced_writer_rollback.unit COMMAND p1673_probe
    guarded-byte-mode-helper-oom "${p1738_sources}/writer-tail-queue.nl" unused)
add_test(NAME p1773_sequential_writer_rollback.unit COMMAND p1673_probe
    guarded-byte-sequential-writer-oom "${p1738_sources}/sequential-named.nl"
    unused)
add_test(NAME p1778_independent_writer_rollback.unit COMMAND p1673_probe
    guarded-byte-distinct-writers-oom "${p1738_sources}/dual-named.nl" unused)
add_test(NAME p1782_observed_mode_rollback.unit COMMAND p1673_probe
    guarded-byte-distinct-writers-oom "${p1738_sources}/mirror-low.nl" unused)
add_test(NAME p1786_displaced_mode_rollback.unit COMMAND p1673_probe
    guarded-byte-distinct-writers-oom "${p1738_sources}/fallback-low.nl" unused)
add_test(NAME p1790_mixed_field_rollback.unit COMMAND p1673_probe
    guarded-byte-distinct-writers-oom "${p1738_sources}/mixed-current-low.nl" unused)
add_test(NAME p1794_snapshot_writer_rollback.unit COMMAND p1673_probe
    guarded-byte-distinct-writers-oom "${p1738_sources}/snapshot-low.nl" unused)
add_test(NAME p1798_normal_writer_completion_rollback.unit COMMAND p1673_probe
    guarded-byte-distinct-writers-oom "${p1738_sources}/complete-displaced-unit.nl" unused)
add_test(NAME p1802_mixed_snapshot_rollback.unit COMMAND p1673_probe
    guarded-byte-distinct-writers-oom "${p1738_sources}/snapshot-mixed.nl" unused)
add_test(NAME p1806_completed_snapshot_rollback.unit COMMAND p1673_probe
    guarded-byte-distinct-writers-oom "${p1738_sources}/complete-snapshot-high-unit.nl" unused)
set_tests_properties(p1738_guarded_scoped_helper_native.integration
    p1738_guarded_scoped_helper_rollback.unit
    p1764_caller_selected_writer_rollback.unit
    p1769_direct_displaced_writer_rollback.unit
    p1778_independent_writer_rollback.unit PROPERTIES TIMEOUT 90)
