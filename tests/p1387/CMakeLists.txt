add_executable(p1387_owned tests/p1387/owned.c)
target_link_libraries(p1387_owned PRIVATE newlang_checked_c_node newlang_semantic newlang_parser)
target_link_options(p1387_owned PRIVATE -Wl,--wrap=malloc -Wl,--wrap=calloc
    -Wl,--wrap=realloc -Wl,--wrap=nl_captured_change_begin
    -Wl,--wrap=nl_captured_change_end)
set(p1387_sources "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1387/sources")
add_test(NAME p1387_owned_nine.unit COMMAND p1387_owned nine "${p1387_sources}/U-proposed-nine.nl")
add_test(NAME p1387_owned_six.unit COMMAND p1387_owned six "${p1387_sources}/I-canonical-six.nl")
add_test(NAME p1387_capture_oom.unit COMMAND p1387_owned oom "${p1387_sources}/U-proposed-nine.nl")
add_test(NAME p1387_validator_oom.unit COMMAND p1387_owned validate-oom "${p1387_sources}/U-proposed-nine.nl")
add_test(NAME p1387_emitter_oom.unit COMMAND p1387_owned emit-oom "${p1387_sources}/U-proposed-nine.nl")
add_test(NAME p1387_source.integration COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1387/cli.py" "$<TARGET_FILE:newlangc>"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1387")
add_test(NAME p1387_native.integration COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1387/native.py" "$<TARGET_FILE:newlangc>"
    "$<TARGET_FILE:five_checked_probe>" "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1387"
    "${CMAKE_C_COMPILER}" "${NEWLANG_SANITIZER}")
