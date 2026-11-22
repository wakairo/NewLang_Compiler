add_executable(p1628_probe tests/p1628/probe.c)
target_include_directories(p1628_probe PRIVATE src)
target_link_libraries(p1628_probe PRIVATE newlang_checked_c_node newlang_semantic)
target_link_options(p1628_probe PRIVATE -Wl,--wrap=malloc)

add_test(NAME p1628_implicit_unit_native.unit COMMAND "${Python3_EXECUTABLE}" -B
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1628/run.py" "$<TARGET_FILE:newlangc>"
    "$<TARGET_FILE:p1623_probe>" "$<TARGET_FILE:p1628_probe>"
    "${CMAKE_CURRENT_BINARY_DIR}/p1628-native")
set_tests_properties(p1628_implicit_unit_native.unit PROPERTIES TIMEOUT 180)

add_test(NAME p1628_entry_identity.unit COMMAND p1628_probe identity
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1628/source/homogeneous-implicit.nl")
add_test(NAME p1628_staging_oom.unit COMMAND p1628_probe staging-oom
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1628/source/mixed-implicit.nl")
