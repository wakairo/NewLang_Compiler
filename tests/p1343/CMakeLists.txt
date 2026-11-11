add_executable(p1343_frame tests/p1343/owned.c)
target_include_directories(p1343_frame PRIVATE src)
target_compile_options(p1343_frame PRIVATE -UNDEBUG)
target_link_libraries(p1343_frame PRIVATE newlang_checked_c_node)
target_link_options(p1343_frame PRIVATE
    -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc
    -Wl,--wrap=nl_copy_body_current -Wl,--wrap=nl_h_custody_seal_result
    -Wl,--wrap=nl_copy_body_event)
foreach(profile A-caller-live B-index1)
    if(profile STREQUAL "A-caller-live")
        set(input 137)
    else()
        set(input 103)
    endif()
    foreach(mode positive oom)
        add_test(NAME p1343_${profile}_${mode}.unit COMMAND p1343_frame ${mode}
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1343/${profile}.nl" ${input})
        set_tests_properties(p1343_${profile}_${mode}.unit PROPERTIES TIMEOUT 180)
    endforeach()
endforeach()
foreach(profile N-wrong-argument N-ended-ref)
    add_test(NAME p1343_${profile}.unit COMMAND p1343_frame negative-semantic
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1343/${profile}.nl" 0)
endforeach()
foreach(profile N-extra-caller)
    add_test(NAME p1343_${profile}.unit COMMAND p1343_frame scope
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1343/${profile}.nl" 0)
endforeach()
add_test(NAME p1343_two_input_calls_current.unit COMMAND p1347_reuse positive
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1343/N-two-input-calls.nl" 0)
add_test(NAME p1343_native.integration COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1343/native.py" "$<TARGET_FILE:newlangc>"
    "${CMAKE_C_COMPILER}" "${CMAKE_CURRENT_BINARY_DIR}/p1343-native"
    "${NEWLANG_SANITIZER}")
add_test(NAME p1343_negative_cli_oom.integration COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1347/failure.py" "$<TARGET_FILE:newlangc>"
    "$<TARGET_FILE:p1332_cli_oom>" "${CMAKE_CURRENT_BINARY_DIR}/p1343-negative" p1343)
set_tests_properties(p1343_native.integration p1343_negative_cli_oom.integration
    PROPERTIES TIMEOUT 180)
