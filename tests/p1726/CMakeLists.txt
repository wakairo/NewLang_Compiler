add_test(NAME p1726_live_descriptor_selector.integration
    COMMAND "${Python3_EXECUTABLE}" -B
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1726/run.py"
    "$<TARGET_FILE:newlangc>" "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1726/source")
set_tests_properties(p1726_live_descriptor_selector.integration PROPERTIES TIMEOUT 90)
