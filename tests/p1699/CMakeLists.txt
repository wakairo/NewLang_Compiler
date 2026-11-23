set(p1699_sources "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1699/source")
add_test(NAME p1699_guarded_product_native.integration COMMAND "${Python3_EXECUTABLE}" -B
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1699/run.py"
    "$<TARGET_FILE:newlangc>" "${p1699_sources}")
add_test(NAME p1699_guarded_product_rollback.unit COMMAND p1673_probe
    guarded-product-oom "${p1699_sources}/unit-product.nl" unused)
add_test(NAME p1699_guarded_receipt_and_seal_oom.unit COMMAND p1492_probe
    oom "${p1699_sources}/unit-product.nl")
set_tests_properties(p1699_guarded_product_rollback.unit
    p1699_guarded_receipt_and_seal_oom.unit PROPERTIES TIMEOUT 90)
