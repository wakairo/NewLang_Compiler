set(p1712_sources "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1712/source")
add_test(NAME p1712_guarded_byte_mode_native.integration COMMAND "${Python3_EXECUTABLE}" -B
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1712/run.py"
    "$<TARGET_FILE:newlangc>" "${p1712_sources}")
add_test(NAME p1712_guarded_byte_mode_rollback.unit COMMAND p1673_probe
    guarded-byte-mode-oom "${p1712_sources}/scalar-1023.nl" unused)
set_tests_properties(p1712_guarded_byte_mode_rollback.unit PROPERTIES TIMEOUT 90)
