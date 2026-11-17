add_executable(p1499_probe tests/p1499/probe.c)
target_include_directories(p1499_probe PRIVATE src)
target_link_libraries(p1499_probe PRIVATE newlang_checked_c_node newlang_semantic newlang_parser)
target_link_options(p1499_probe PRIVATE -Wl,--wrap=nl_guarded_usize_capture
    -Wl,--wrap=nl_h_custody_seal_result -Wl,--wrap=nl_sem_clone
    -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc)
foreach(case A-offset-composition B-repeat-advance)
    add_test(NAME p1499_${case}_owned.unit COMMAND p1499_probe composition
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1499/${case}.nl")
endforeach()
if(BUILD_TESTING)
    find_package(Python3 3.12 REQUIRED COMPONENTS Interpreter)
    add_test(NAME p1499_guarded_usize_native.integration
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1499/native.py"
        "$<TARGET_FILE:newlangc>" "${CMAKE_C_COMPILER}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1499"
        "${CMAKE_CURRENT_BINARY_DIR}/p1499-native" "${NEWLANG_SANITIZER}")
    set_tests_properties(p1499_guarded_usize_native.integration PROPERTIES TIMEOUT 180 RUN_SERIAL TRUE)
endif()
add_test(NAME p1499_current_result_tamper.unit COMMAND p1499_probe attack
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1499/A-offset-composition.nl")
add_test(NAME p1499_selected_oom.unit COMMAND p1499_probe oom
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1499/A-offset-composition.nl")
add_test(NAME p1499_named_no_clobber.unit COMMAND p1499_probe named
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1499/A-offset-composition.nl"
    "${CMAKE_CURRENT_BINARY_DIR}/p1499-named-output.c")
set_tests_properties(p1499_current_result_tamper.unit p1499_selected_oom.unit
    p1499_named_no_clobber.unit PROPERTIES RUN_SERIAL TRUE)
add_test(NAME p1499_N-changed-formal.unit COMMAND p1499_probe changed-negative
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1499/N-changed-formal.nl")
foreach(case N-overflow-consumer N-wrong-formal)
    add_test(NAME p1499_${case}.unit COMMAND p1499_probe negative
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1499/${case}.nl")
endforeach()
