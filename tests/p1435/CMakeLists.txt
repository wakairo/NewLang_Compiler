add_executable(p1435_probe tests/p1435/probe.c)
target_link_libraries(p1435_probe PRIVATE newlang_checked_c_node
    newlang_semantic newlang_parser)
target_link_options(p1435_probe PRIVATE -Wl,--wrap=malloc
    -Wl,--wrap=realloc -Wl,--wrap=nl_sem_clone)
set(p1435_sources
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1435/C-mixed-payload.nl"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1435/P-some-bool.nl"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1435/P-none-bool-copy.nl")
add_test(NAME p1435_owned.unit COMMAND p1435_probe owned ${p1435_sources})
add_test(NAME p1435_emit.unit COMMAND p1435_probe emit ${p1435_sources})
add_test(NAME p1435_tamper.unit COMMAND p1435_probe tamper
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1435/P-some-bool.nl"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1435/P-none-bool-copy.nl")
foreach(mode domain loan)
    if(mode STREQUAL "domain")
        set(p1435_negative N-wrong-domain.nl)
    else()
        set(p1435_negative N-active-loan-end.nl)
    endif()
    add_test(NAME p1435_${mode}_rollback.unit COMMAND p1435_probe negative
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1435/${p1435_negative}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1435/C-mixed-payload.nl")
endforeach()
foreach(point RANGE 1 3)
    add_test(NAME p1435_oom_${point}.unit COMMAND p1435_probe oom${point}
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1435/P-some-bool.nl")
endforeach()
