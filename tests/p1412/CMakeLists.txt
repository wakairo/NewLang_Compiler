add_executable(p1412_seal tests/p1412/seal.c)
target_link_libraries(p1412_seal PRIVATE newlang_checked_c_node newlang_semantic newlang_parser)
target_link_options(p1412_seal PRIVATE -Wl,--wrap=malloc -Wl,--wrap=realloc
    -Wl,--wrap=nl_sem_copy_value -Wl,--wrap=nl_h_custody_seal_result)
add_test(NAME p1412_result_seal.unit COMMAND p1412_seal positive
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1408/S-v1-same-original-two-paths.nl")
add_test(NAME p1412_result_faults.unit COMMAND p1412_seal faults
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1408/S-v1-same-original-two-paths.nl")
add_test(NAME p1412_different_original.unit COMMAND p1412_seal negative
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1408/N-v1-divergent-originals.nl")
add_executable(p1412_emitter tests/p1412/emitter.c)
target_link_libraries(p1412_emitter PRIVATE newlang_checked_c_node newlang_semantic newlang_parser)
target_link_options(p1412_emitter PRIVATE -Wl,--wrap=malloc -Wl,--wrap=realloc
    -Wl,--wrap=nl_sem_copy_value)
add_test(NAME p1412_C17_false.unit COMMAND p1412_emitter emit
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1408/S-v1-same-original-two-paths.nl")
add_test(NAME p1412_C17_true.unit COMMAND p1412_emitter emit
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1412/T-v1-true-arm.nl")
add_test(NAME p1412_C17_faults.unit COMMAND p1412_emitter faults
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1408/S-v1-same-original-two-paths.nl")
