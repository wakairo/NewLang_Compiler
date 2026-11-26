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
add_test(NAME p1810_distinct_registered_writers_rollback.unit COMMAND p1673_probe
    guarded-byte-distinct-writers-oom "${p1738_sources}/split-literal-unit.nl" unused)
add_test(NAME p1814_saved_recovery_rollback.unit COMMAND p1673_probe
    guarded-byte-sequential-writer-oom "${p1738_sources}/restore-low.nl" unused)
add_test(NAME p1817_midpoint_rollback.unit COMMAND p1673_probe
    guarded-byte-sequential-writer-oom "${p1738_sources}/report-low.nl" unused)
add_test(NAME p1822_saved_midpoint_rollback.unit COMMAND p1673_probe
    guarded-byte-sequential-writer-oom "${p1738_sources}/saved-report-low.nl" unused)
add_test(NAME p1825_routed_report_rollback.unit COMMAND p1673_probe
    guarded-byte-sequential-writer-oom "${p1738_sources}/payload-low.nl" unused)
add_test(NAME p1829_saved_route_rollback.unit COMMAND p1673_probe
    guarded-byte-sequential-writer-oom "${p1738_sources}/saved-route-low-edited.nl" unused)
add_test(NAME p1829_saved_payload_rollback.unit COMMAND p1673_probe
    guarded-byte-sequential-writer-oom "${p1738_sources}/saved-payload-low-edited.nl" unused)
add_test(NAME p1833_report_handoff_rollback.unit COMMAND p1673_probe
    guarded-byte-sequential-writer-oom "${p1738_sources}/report-handoff-low.nl" unused)
add_test(NAME p1833_report_delivery_rollback.unit COMMAND p1673_probe
    guarded-byte-sequential-writer-oom "${p1738_sources}/report-publish-low-before.nl" unused)
set_tests_properties(p1738_guarded_scoped_helper_native.integration
    p1738_guarded_scoped_helper_rollback.unit
    p1764_caller_selected_writer_rollback.unit
    p1769_direct_displaced_writer_rollback.unit
    p1778_independent_writer_rollback.unit PROPERTIES TIMEOUT 90)
