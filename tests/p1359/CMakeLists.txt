add_executable(p1359_wrapper tests/p1359/owned.c)
target_include_directories(p1359_wrapper PRIVATE src)
target_compile_options(p1359_wrapper PRIVATE -UNDEBUG)
target_link_libraries(p1359_wrapper PRIVATE newlang_checked_c_node)
target_link_options(p1359_wrapper PRIVATE
    -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc
    -Wl,--wrap=nl_copy_body_current -Wl,--wrap=nl_h_custody_seal_result
    -Wl,--wrap=nl_copy_body_event -Wl,--wrap=nl_copy_body_seal
    -Wl,--wrap=nl_copy_body_readonly)
foreach(profile A-two-inputs B-reversed-field)
    if(profile STREQUAL "A-two-inputs")
        set(index 1)
    else()
        set(index 0)
    endif()
    foreach(mode positive oom)
        add_test(NAME p1359_${profile}_${mode}.unit COMMAND p1359_wrapper ${mode}
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1359/${profile}.nl" ${index})
        set_tests_properties(p1359_${profile}_${mode}.unit PROPERTIES TIMEOUT 180)
    endforeach()
endforeach()
add_test(NAME p1359_semantic_negative.unit COMMAND p1359_wrapper negative
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1359/N-ended-loan.nl"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1359/N-wrong-argument.nl")
add_test(NAME p1359_N-third-call.unit COMMAND p1359_wrapper precision
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1359/N-third-call.nl")
add_test(NAME p1359_native.integration COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1359/native.py" "$<TARGET_FILE:newlangc>"
    "${CMAKE_C_COMPILER}" "${CMAKE_CURRENT_BINARY_DIR}/p1359-native"
    "${NEWLANG_SANITIZER}")
add_test(NAME p1359_negative_cli_oom.integration COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1359/failure.py" "$<TARGET_FILE:newlangc>"
    "$<TARGET_FILE:p1332_cli_oom>" "${CMAKE_CURRENT_BINARY_DIR}/p1359-negative" p1359)
add_test(NAME p1359_legacy_C_origin.integration COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1359/compatibility.py" "$<TARGET_FILE:newlangc>"
    "${CMAKE_CURRENT_BINARY_DIR}/p1359-compat")
set_tests_properties(p1359_native.integration p1359_negative_cli_oom.integration
    p1359_legacy_C_origin.integration PROPERTIES TIMEOUT 180)
