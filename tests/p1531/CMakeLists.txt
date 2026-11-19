add_test(NAME p1531_direct_prefix_scope.integration COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1531/scope.py" "$<TARGET_FILE:newlangc>"
    "$<TARGET_FILE:p1529_consumer>" "$<TARGET_FILE:p1481_probe>"
    "$<TARGET_FILE:p1529_public_owned>" "${CMAKE_CURRENT_SOURCE_DIR}/tests"
    "${CMAKE_CURRENT_BINARY_DIR}/p1531-scope")
set_tests_properties(p1531_direct_prefix_scope.integration PROPERTIES TIMEOUT 180 RUN_SERIAL TRUE)
