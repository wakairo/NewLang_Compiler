add_executable(p1420_check tests/p1420/check.c)
target_link_libraries(p1420_check PRIVATE newlang_checked_c_node newlang_semantic newlang_parser)
target_link_options(p1420_check PRIVATE -Wl,--wrap=malloc -Wl,--wrap=realloc
    -Wl,--wrap=nl_h_unchecked_prepare -Wl,--wrap=nl_sem_new_value
    -Wl,--wrap=nl_sem_copy_value -Wl,--wrap=nl_h_custody_seal_result
    -Wl,--wrap=nl_h_unchecked_machine_only -Wl,--wrap=nl_unchecked_domain_target)
foreach(source U Alias)
    add_test(NAME p1420_${source}_owned.unit COMMAND p1420_check positive
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1420/${source}.nl")
endforeach()
foreach(source S T)
    if(source STREQUAL "S")
        set(p1420_machine_source "tests/p1408/S-v1-same-original-two-paths.nl")
    else()
        set(p1420_machine_source "tests/p1412/T-v1-true-arm.nl")
    endif()
    add_test(NAME p1420_${source}_machine.unit COMMAND p1420_check machine
        "${CMAKE_CURRENT_SOURCE_DIR}/${p1420_machine_source}")
endforeach()
foreach(mode faults coarse)
    add_test(NAME p1420_${mode}.unit COMMAND p1420_check ${mode}
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1420/U.nl")
endforeach()
add_test(NAME p1420_P_precision.unit COMMAND p1420_check negative
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1420/P.nl" P13-JOIN-PRECISION)
add_test(NAME p1420_N_precision.unit COMMAND p1420_check negative
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1408/N-v1-divergent-originals.nl" P13-JOIN-PRECISION)
add_test(NAME p1420_wrong_domain.unit COMMAND p1420_check negative
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1420/wrong-domain.nl" ALLOCATED-DOMAIN-MISMATCH)
add_test(NAME p1420_ended_domain.unit COMMAND p1420_check negative
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1420/ended-domain.nl" P3-USE-AFTER-CONSUME)
add_test(NAME p1420_escape.unit COMMAND p1420_check negative
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1420/escape.nl" UNCHECKED-REF-USE)
add_test(NAME p1420_write_source.unit COMMAND p1420_check syntax
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1420/write.nl" UNCHECKED-READ-MODE)
add_test(NAME p1420_missing_stable_source.unit COMMAND p1420_check syntax
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1420/missing-stable.nl" ALLOCATED-CALL-ARITY)
foreach(event RANGE 1 6)
    add_test(NAME p1420_oom_${event}.unit COMMAND p1420_check oom
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1420/U.nl" ${event})
endforeach()
