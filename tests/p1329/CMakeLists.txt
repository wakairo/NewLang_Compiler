add_executable(p1329_current_copy tests/p1329/owned.c)
target_include_directories(p1329_current_copy PRIVATE src)
target_compile_options(p1329_current_copy PRIVATE -UNDEBUG)
target_link_libraries(p1329_current_copy PRIVATE newlang_checked_c_node)
target_link_options(p1329_current_copy PRIVATE
    -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc
    -Wl,--wrap=nl_copy_body_current -Wl,--wrap=nl_h_custody_seal_result)
foreach(profile A-callee-local B-index1)
    if(profile STREQUAL "A-callee-local")
        set(index 0)
    else()
        set(index 1)
    endif()
    add_test(NAME p1329_${profile}_owned.unit COMMAND p1329_current_copy
        positive "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1329/${profile}.nl" ${index})
endforeach()
add_test(NAME p1329_semantic_negative.unit COMMAND p1329_current_copy negative
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1329/N-type.nl"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1329/N-ended-ref.nl")
add_test(NAME p1329_current_copy_oom.unit COMMAND p1329_current_copy oom
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1329/A-callee-local.nl")
add_test(NAME p1329_scope_precision.unit COMMAND $<TARGET_FILE:p1335_two_calls> precision
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1329/N-two-originals.nl")
set_tests_properties(p1329_A-callee-local_owned.unit p1329_B-index1_owned.unit
    p1329_semantic_negative.unit p1329_current_copy_oom.unit
    p1329_scope_precision.unit PROPERTIES TIMEOUT 180)
