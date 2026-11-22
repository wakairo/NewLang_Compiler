add_executable(p1632_probe tests/p1632/probe.c)
target_include_directories(p1632_probe PRIVATE src)
target_link_libraries(p1632_probe PRIVATE newlang_checked_c_node newlang_semantic)
target_link_options(p1632_probe PRIVATE -Wl,--wrap=malloc
    -Wl,--wrap=nl_h_custody_seal_result)

add_test(NAME p1632_named_bool_discard.unit COMMAND "${Python3_EXECUTABLE}" -B
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1632/run.py" "$<TARGET_FILE:newlangc>"
    "$<TARGET_FILE:p1623_probe>" "$<TARGET_FILE:p1632_probe>"
    "${CMAKE_CURRENT_BINARY_DIR}/p1632-native")
set_tests_properties(p1632_named_bool_discard.unit PROPERTIES TIMEOUT 180)

add_test(NAME p1632_entry-world-oom.unit COMMAND p1632_probe entry-world-oom
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1632/source/caller-discard.nl")
add_test(NAME p1632_helper-seal-oom.unit COMMAND p1632_probe helper-seal-oom
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1632/source/helper-discard.nl")
add_test(NAME p1632_staging-oom.unit COMMAND p1632_probe staging-oom
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1632/source/helper-discard.nl")
