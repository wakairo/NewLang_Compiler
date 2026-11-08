# One forward apparatus repair; C275 frozen wrappers/history remain intact.
add_executable(p1217_c276_p733_current tests/p1217/c276/p733_current_v2.c)
target_compile_options(p1217_c276_p733_current PRIVATE -fno-pie)
target_link_options(p1217_c276_p733_current PRIVATE -no-pie
    -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc)
target_link_libraries(p1217_c276_p733_current PRIVATE
    newlang_semantic newlang_parser newlang_checked_c_node)
