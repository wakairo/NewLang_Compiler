add_executable(p1339_readonly tests/p1339/owned.c)
target_include_directories(p1339_readonly PRIVATE src)
target_compile_options(p1339_readonly PRIVATE -UNDEBUG)
target_link_libraries(p1339_readonly PRIVATE newlang_checked_c_node)
target_link_options(p1339_readonly PRIVATE
    -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc
    -Wl,--wrap=nl_copy_body_current -Wl,--wrap=nl_h_custody_seal_result
    -Wl,--wrap=nl_copy_body_event)
foreach(profile A-input67 B-input109)
    if(profile STREQUAL "A-input67")
        set(input 67)
    else()
        set(input 109)
    endif()
    foreach(mode positive oom)
        add_test(NAME p1339_${profile}_${mode}.unit COMMAND p1339_readonly ${mode}
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1339/${profile}.nl" ${input})
        set_tests_properties(p1339_${profile}_${mode}.unit PROPERTIES TIMEOUT 180)
    endforeach()
endforeach()
foreach(negative N-argument-type N-ended-ref)
    add_test(NAME p1339_${negative}.unit COMMAND p1339_readonly negative-semantic
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1339/${negative}.nl" 0)
endforeach()
add_test(NAME p1339_readonly_formal_precision.unit COMMAND p1339_readonly negative-precision
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1339/N-formal-write.nl" 0)
add_test(NAME p1339_two_input_calls_precision.unit COMMAND p1339_readonly scope
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1339/N-two-input-calls.nl" 0)
