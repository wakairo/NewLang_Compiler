add_executable(p1408_owned tests/p1408/owned.c)
target_link_libraries(p1408_owned PRIVATE newlang_checked_c_node newlang_semantic newlang_parser)
target_link_options(p1408_owned PRIVATE -Wl,--wrap=realloc -Wl,--wrap=nl_sem_copy_value)
# The original owned.c and its C17-unsupported oracle are retained verbatim.
# Current CHECK/owned facts plus the now-supported C17 path have prospective
# successors p1412_result_seal / p1412_C17_false; the historical oracle runs
# on its preserved C1 revision, not against a different backend capability.
add_test(NAME p1408_different_ptr_precision.unit COMMAND p1408_owned negative
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1408/N-v1-divergent-originals.nl")
add_test(NAME p1408_ptr_copy_oom.unit COMMAND p1408_owned oom
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1408/S-v1-same-original-two-paths.nl")
