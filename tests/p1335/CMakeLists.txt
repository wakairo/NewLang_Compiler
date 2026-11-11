add_executable(p1335_two_calls tests/p1335/owned.c)
target_include_directories(p1335_two_calls PRIVATE src)
target_compile_options(p1335_two_calls PRIVATE -UNDEBUG)
target_link_libraries(p1335_two_calls PRIVATE newlang_checked_c_node)
target_link_options(p1335_two_calls PRIVATE
    -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc
    -Wl,--wrap=nl_copy_body_current -Wl,--wrap=nl_h_custody_seal_result
    -Wl,--wrap=nl_copy_body_event)
foreach(profile A-two-calls B-two-calls-index1)
    if(profile STREQUAL "A-two-calls")
        set(index 0)
    else()
        set(index 1)
    endif()
    add_test(NAME p1335_${profile}_owned.unit COMMAND p1335_two_calls positive
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1335/${profile}.nl" ${index})
    add_test(NAME p1335_${profile}_oom.unit COMMAND p1335_two_calls oom
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1335/${profile}.nl" ${index})
endforeach()
