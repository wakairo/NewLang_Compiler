add_executable(p1393_owned tests/p1393/owned.c)
target_link_libraries(p1393_owned PRIVATE newlang_checked_c_node newlang_semantic newlang_parser)
add_test(NAME p1393_owned.unit COMMAND p1393_owned owned "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1393/sources/P1-C-free-current-tail-read-v2.nl")
target_link_options(p1393_owned PRIVATE -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc
    -Wl,--wrap=nl_captured_read_begin -Wl,--wrap=nl_captured_read_end -Wl,--wrap=nl_sem_clone
    -Wl,--wrap=nl_h_custody_seal_result)
add_test(NAME p1393_temporal_oom.unit COMMAND p1393_owned oom "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1393/sources/P1-C-free-current-tail-read-v2.nl")
add_executable(p1393_probe tests/p1393/probe.c)
target_link_libraries(p1393_probe PRIVATE newlang_semantic newlang_parser)
add_test(NAME p1393_source.integration COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1393/cli.py" "$<TARGET_FILE:newlangc>" "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1393")
add_test(NAME p1393_native.integration COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1393/native.py" "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:p1393_probe>"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1393" "${CMAKE_C_COMPILER}" "${NEWLANG_SANITIZER}")
add_test(NAME p1393_current_read_auth.unit COMMAND p1393_owned auth "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1393/sources/P1-C-free-current-tail-read-v2.nl")

set(p1393_dir "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1393")
set(p1393_retained_live "${p1393_dir}/sources/P2-C-live-current-tail-read.nl")
add_test(NAME p1393_retained_live_source.integration COMMAND "${Python3_EXECUTABLE}"
    "${p1393_dir}/retained_live.py" "$<TARGET_FILE:newlangc>"
    "$<TARGET_FILE:p1393_owned>" "${p1393_retained_live}")
add_test(NAME p1393_retained_live_oom.unit COMMAND p1393_owned oom "${p1393_retained_live}")
add_test(NAME p1393_retained_live_native.integration COMMAND "${Python3_EXECUTABLE}"
    "${p1393_dir}/native.py" "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:p1393_probe>"
    "${p1393_dir}" "${CMAKE_C_COMPILER}" "${NEWLANG_SANITIZER}" --retained-live)

set(p1863_shared_writer "${p1393_dir}/sources/P3-C-live-shared-tail-writer.nl")
add_test(NAME p1863_shared_writer_owned.unit
    COMMAND p1393_probe "${p1863_shared_writer}" --shared-writer)
add_test(NAME p1863_shared_writer_native.integration COMMAND "${Python3_EXECUTABLE}"
    "${p1393_dir}/native.py" "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:p1393_probe>"
    "${p1393_dir}" "${CMAKE_C_COMPILER}" "${NEWLANG_SANITIZER}" --shared-writer)
add_test(NAME p1863_shared_writer_oom.unit
    COMMAND p1393_owned writer-oom "${p1863_shared_writer}")
add_test(NAME p1863_shared_writer_source.integration COMMAND "${Python3_EXECUTABLE}"
    "${p1393_dir}/retained_live.py" "$<TARGET_FILE:newlangc>"
    "$<TARGET_FILE:p1393_probe>" "${p1863_shared_writer}" --shared-writer)
