set(p1718_sources "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1718/source")
add_test(NAME p1718_guarded_late_descriptor_native.integration COMMAND "${Python3_EXECUTABLE}" -B
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1718/run.py"
    "$<TARGET_FILE:newlangc>" "${p1718_sources}")
add_test(NAME p1718_guarded_late_descriptor_rollback.unit COMMAND p1673_probe
    guarded-byte-mode-oom "${p1718_sources}/guarded-late-1023.nl" unused)
set_tests_properties(p1718_guarded_late_descriptor_native.integration
    p1718_guarded_late_descriptor_rollback.unit PROPERTIES TIMEOUT 90)
