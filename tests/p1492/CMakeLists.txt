add_executable(p1492_probe tests/p1492/probe.c)
target_include_directories(p1492_probe PRIVATE src)
target_link_libraries(p1492_probe PRIVATE newlang_checked_c_node newlang_semantic newlang_parser)
target_link_options(p1492_probe PRIVATE -Wl,--wrap=nl_guarded_usize_capture
    -Wl,--wrap=nl_h_custody_seal_result -Wl,--wrap=nl_sem_clone
    -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc)
foreach(case Q-next-occupancy B-batch-advance M-max-bound)
    add_test(NAME p1492_${case}_owned.unit COMMAND p1492_probe owned
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1492/${case}.nl")
endforeach()
if(BUILD_TESTING)
    find_package(Python3 3.12 REQUIRED COMPONENTS Interpreter)
    add_test(NAME p1492_guarded_usize_native.integration
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1492/native.py"
        "$<TARGET_FILE:newlangc>" "${CMAKE_C_COMPILER}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1492"
        "${CMAKE_CURRENT_BINARY_DIR}/p1492-native" "${NEWLANG_SANITIZER}")
    set_tests_properties(p1492_guarded_usize_native.integration PROPERTIES TIMEOUT 180 RUN_SERIAL TRUE)
endif()
add_test(NAME p1492_current_result_tamper.unit COMMAND p1492_probe tamper
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1492/Q-next-occupancy.nl")
add_test(NAME p1492_selected_oom.unit COMMAND p1492_probe oom
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1492/Q-next-occupancy.nl")
add_test(NAME p1492_named_no_clobber.unit COMMAND p1492_probe named
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1492/Q-next-occupancy.nl"
    "${CMAKE_CURRENT_BINARY_DIR}/p1492-named-output.c")
set_tests_properties(p1492_current_result_tamper.unit p1492_selected_oom.unit
    p1492_named_no_clobber.unit PROPERTIES RUN_SERIAL TRUE)
foreach(case N-decoy-guard N-overflow-bound)
    add_test(NAME p1492_${case}.unit COMMAND p1492_probe negative
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1492/${case}.nl")
endforeach()
