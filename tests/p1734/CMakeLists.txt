add_test(NAME p1734_scoped_update_helper.integration
    COMMAND "${Python3_EXECUTABLE}" -B
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1734/run.py"
    "$<TARGET_FILE:newlangc>" "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1734/source")
set_tests_properties(p1734_scoped_update_helper.integration PROPERTIES TIMEOUT 90)
