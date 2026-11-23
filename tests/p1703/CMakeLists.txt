set(p1703_sources "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1703/source")
add_test(NAME p1703_guarded_addition_decision_native.integration COMMAND "${Python3_EXECUTABLE}" -B
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1703/run.py"
    "$<TARGET_FILE:newlangc>" "${p1703_sources}")
add_test(NAME p1703_guarded_addition_decision_rollback.unit COMMAND p1673_probe
    guarded-addition-decision-oom "${p1703_sources}/long-queue.nl" unused)
add_test(NAME p1703_guarded_addition_receipt_and_seal_oom.unit COMMAND p1492_probe
    oom "${p1703_sources}/long-queue.nl")
set_tests_properties(p1703_guarded_addition_decision_rollback.unit
    p1703_guarded_addition_receipt_and_seal_oom.unit PROPERTIES TIMEOUT 90)
