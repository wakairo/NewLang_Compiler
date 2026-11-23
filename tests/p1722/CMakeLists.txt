add_test(NAME p1722_live_descriptor.integration COMMAND "${Python3_EXECUTABLE}" -B
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1722/run.py"
    "$<TARGET_FILE:newlangc>" "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1722/source")
set_tests_properties(p1722_live_descriptor.integration PROPERTIES TIMEOUT 90)
