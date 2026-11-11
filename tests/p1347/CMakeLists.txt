add_executable(p1347_reuse tests/p1347/owned.c)
target_include_directories(p1347_reuse PRIVATE src)
target_compile_options(p1347_reuse PRIVATE -UNDEBUG)
target_link_libraries(p1347_reuse PRIVATE newlang_checked_c_node)
target_link_options(p1347_reuse PRIVATE
    -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc
    -Wl,--wrap=nl_copy_body_current -Wl,--wrap=nl_h_custody_seal_result
    -Wl,--wrap=nl_copy_body_event)
foreach(profile A-updated-caller B-reverse-field)
    if(profile STREQUAL "A-updated-caller")
        set(index 0)
    else()
        set(index 1)
    endif()
    foreach(mode positive oom)
        add_test(NAME p1347_${profile}_${mode}.unit COMMAND p1347_reuse ${mode}
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1347/${profile}.nl" ${index})
        set_tests_properties(p1347_${profile}_${mode}.unit PROPERTIES TIMEOUT 180)
    endforeach()
endforeach()
add_test(NAME p1347_semantic_negative.unit COMMAND p1347_reuse negative
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1347/N-wrong-type.nl"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1347/N-ended-loan.nl")
foreach(profile N-third-call N-extra-original)
    add_test(NAME p1347_${profile}.unit COMMAND p1347_reuse precision
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1347/${profile}.nl")
endforeach()
