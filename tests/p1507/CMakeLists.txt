add_executable(p1507_probe tests/p1507/probe.c)
target_include_directories(p1507_probe PRIVATE src)
target_link_libraries(p1507_probe PRIVATE newlang_checked_c_node newlang_semantic newlang_parser)
target_link_options(p1507_probe PRIVATE -Wl,--wrap=malloc -Wl,--wrap=realloc -Wl,--wrap=nl_sem_clone)
foreach(case A-direct-zero B-direct-index-one)
    add_test(NAME p1507_${case}_owned.unit COMMAND p1507_probe
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1507/${case}.nl")
endforeach()
add_test(NAME p1507_direct_native.integration
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1507/native.py"
    "$<TARGET_FILE:newlangc>" "${CMAKE_C_COMPILER}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1507"
    "${CMAKE_CURRENT_BINARY_DIR}/p1507-native" "${NEWLANG_SANITIZER}")
set_tests_properties(p1507_direct_native.integration PROPERTIES TIMEOUT 180 RUN_SERIAL TRUE)
add_test(NAME p1507_named_no_clobber.integration
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1507/cli.py"
    "$<TARGET_FILE:newlangc>" "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1507"
    "${CMAKE_CURRENT_BINARY_DIR}/p1507-named")
add_test(NAME p1507_direct_tamper.unit COMMAND p1507_probe attack
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1507/A-direct-zero.nl")
foreach(case N-double-free N-loan-escape N-missing-cleanup N-stale-read
             N-wrong-allocation N-wrong-domain N-wrong-formal N-wrong-original)
    add_test(NAME p1507_${case}.unit COMMAND p1507_probe negative
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1507/${case}.nl")
endforeach()
foreach(point RANGE 1 5)
    add_test(NAME p1507_oom_${point}.unit COMMAND p1507_probe oom
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1507/A-direct-zero.nl" ${point})
endforeach()
