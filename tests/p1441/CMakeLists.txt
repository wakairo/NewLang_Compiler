add_executable(p1441_probe tests/p1441/probe.c)
target_link_libraries(p1441_probe PRIVATE newlang_checked_c_node newlang_semantic newlang_parser)
target_link_options(p1441_probe PRIVATE -Wl,--wrap=malloc -Wl,--wrap=realloc
    -Wl,--wrap=nl_sem_clone -Wl,--wrap=nl_h_custody_seal_result)
set(p1441_base "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1441")
set(p1441_sources "${p1441_base}/C-direct.nl" "${p1441_base}/P-some-read.nl"
    "${p1441_base}/P-none-read.nl")
foreach(mode owned emit)
    add_test(NAME p1441_${mode}.unit COMMAND p1441_probe ${mode} ${p1441_sources})
endforeach()
add_test(NAME p1441_world_auth.unit COMMAND p1441_probe auth
    "${p1441_base}/P-some-read.nl" "${p1441_base}/P-none-read.nl")
add_test(NAME p1441_foreign_arm.unit COMMAND p1441_probe foreign
    "${p1441_base}/P-some-read.nl" "${p1441_base}/P-none-read.nl")
add_test(NAME p1441_read_metadata.unit COMMAND p1441_probe read-metadata
    "${p1441_base}/P-some-read.nl")
foreach(negative wrong-domain loan-escape owner-consumed)
    add_test(NAME p1441_${negative}.unit COMMAND p1441_probe negative
        "${p1441_base}/N-${negative}.nl" "${p1441_base}/C-direct.nl")
endforeach()
foreach(point RANGE 1 3)
    add_test(NAME p1441_oom_${point}.unit COMMAND p1441_probe oom${point}
        "${p1441_base}/P-some-read.nl")
endforeach()
add_test(NAME p1441_snapshot_oom.unit COMMAND p1441_probe snapshot-oom
    "${p1441_base}/P-some-read.nl")
add_test(NAME p1441_cli_native.integration COMMAND "${Python3_EXECUTABLE}"
    "${p1441_base}/native.py" "$<TARGET_FILE:newlangc>" "${CMAKE_C_COMPILER}"
    "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/p1441-native")
set_tests_properties(p1441_cli_native.integration PROPERTIES TIMEOUT 180)
