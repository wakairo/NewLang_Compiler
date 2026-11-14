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
