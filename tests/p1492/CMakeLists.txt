add_executable(p1492_probe tests/p1492/probe.c)
target_include_directories(p1492_probe PRIVATE src)
target_link_libraries(p1492_probe PRIVATE newlang_semantic newlang_parser)
foreach(case Q-next-occupancy B-batch-advance M-max-bound)
    add_test(NAME p1492_${case}_owned.unit COMMAND p1492_probe owned
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1492/${case}.nl")
endforeach()
foreach(case N-decoy-guard N-overflow-bound)
    add_test(NAME p1492_${case}.unit COMMAND p1492_probe negative
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1492/${case}.nl")
endforeach()
