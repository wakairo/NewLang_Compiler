add_executable(p1372_owned tests/p1372/owned.c)
target_include_directories(p1372_owned PRIVATE src)
target_compile_options(p1372_owned PRIVATE -UNDEBUG)
target_link_libraries(p1372_owned PRIVATE newlang_checked_c_node)
target_link_options(p1372_owned PRIVATE
    -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc
    -Wl,--wrap=nl_sem_clone -Wl,--wrap=nl_body_create_span
    -Wl,--wrap=nl_h_custody_seal_result)
foreach(profile L-straight-local-old B-field-zero-local-alias)
    if(profile STREQUAL "L-straight-local-old")
        set(index 1)
    else()
        set(index 0)
    endif()
    foreach(mode positive registration-oom oom copy-oom)
        add_test(NAME p1372_${profile}_${mode}.unit COMMAND p1372_owned ${mode}
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1372/${profile}.nl" ${index})
        set_tests_properties(p1372_${profile}_${mode}.unit PROPERTIES TIMEOUT 180)
    endforeach()
endforeach()
add_test(NAME p1372_loan_negative.unit COMMAND p1372_owned negative-loan
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1372/N-local-loan-escape.nl" 1)
add_test(NAME p1372_type_negative.unit COMMAND p1372_owned negative-type
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1372/N-wrong-actual.nl" 1)
