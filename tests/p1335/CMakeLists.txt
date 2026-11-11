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
add_test(NAME p1335_semantic_negative.unit COMMAND p1335_two_calls negative
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1329/N-type.nl"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1329/N-ended-ref.nl")
add_test(NAME p1335_negative_cli_oom.integration COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1335/negative.py" "$<TARGET_FILE:newlangc>"
    "$<TARGET_FILE:p1332_cli_oom>" "${CMAKE_CURRENT_BINARY_DIR}/p1335-negative")
add_test(NAME p1335_native.integration COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1335/native.py" "$<TARGET_FILE:newlangc>"
    "${CMAKE_C_COMPILER}" "${CMAKE_CURRENT_BINARY_DIR}/p1335-native")
set_tests_properties(p1335_A-two-calls_owned.unit p1335_B-two-calls-index1_owned.unit
    p1335_A-two-calls_oom.unit p1335_B-two-calls-index1_oom.unit
    p1335_semantic_negative.unit p1335_negative_cli_oom.integration
    p1335_native.integration PROPERTIES TIMEOUT 180)
