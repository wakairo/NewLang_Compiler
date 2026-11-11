add_executable(p1351_wrapper tests/p1351/owned.c)
target_include_directories(p1351_wrapper PRIVATE src)
target_compile_options(p1351_wrapper PRIVATE -UNDEBUG)
target_link_libraries(p1351_wrapper PRIVATE newlang_checked_c_node)
target_link_options(p1351_wrapper PRIVATE
    -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc
    -Wl,--wrap=nl_copy_body_current -Wl,--wrap=nl_h_custody_seal_result
    -Wl,--wrap=nl_copy_body_event -Wl,--wrap=nl_copy_body_seal)
foreach(profile A-wrapper B-wrapper-index1)
    if(profile STREQUAL "A-wrapper")
        set(index 0)
    else()
        set(index 1)
    endif()
    foreach(mode positive oom)
        add_test(NAME p1351_${profile}_${mode}.unit COMMAND p1351_wrapper ${mode}
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1351/${profile}.nl" ${index})
        set_tests_properties(p1351_${profile}_${mode}.unit PROPERTIES TIMEOUT 180)
    endforeach()
endforeach()
add_test(NAME p1351_semantic_negative.unit COMMAND p1351_wrapper negative
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1351/N-ended-loan.nl"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1351/N-wrong-type.nl")
foreach(profile N-two-inner N-extra-wrapper)
    add_test(NAME p1351_${profile}.unit COMMAND p1351_wrapper precision
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1351/${profile}.nl")
endforeach()
