add_executable(p1368_owned tests/p1368/owned.c)
target_include_directories(p1368_owned PRIVATE src)
target_compile_options(p1368_owned PRIVATE -UNDEBUG)
target_link_libraries(p1368_owned PRIVATE newlang_checked_c_node)
target_link_options(p1368_owned PRIVATE
    -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc
    -Wl,--wrap=nl_sem_clone -Wl,--wrap=nl_body_create_span
    -Wl,--wrap=nl_h_custody_seal_result)
foreach(profile A-current-copy-call B-field-zero-alias)
    if(profile STREQUAL "A-current-copy-call")
        set(index 1)
    else()
        set(index 0)
    endif()
    foreach(mode positive registration-oom oom)
        add_test(NAME p1368_${profile}_${mode}.unit COMMAND p1368_owned ${mode}
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1368/${profile}.nl" ${index})
        set_tests_properties(p1368_${profile}_${mode}.unit PROPERTIES TIMEOUT 180)
    endforeach()
endforeach()
add_test(NAME p1368_loan_negative.unit COMMAND p1368_owned negative-loan
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1368/N-iteration-loan-escape.nl" 1)
add_test(NAME p1368_type_negative.unit COMMAND p1368_owned negative-type
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1368/N-wrong-actual.nl" 1)
add_test(NAME p1368_native.integration COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1368/native.py" "$<TARGET_FILE:newlangc>"
    "${CMAKE_C_COMPILER}" "${CMAKE_CURRENT_BINARY_DIR}/p1368-native"
    "${NEWLANG_SANITIZER}")
add_test(NAME p1368_negative_cli_oom.integration COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1368/failure.py" "$<TARGET_FILE:newlangc>"
    "$<TARGET_FILE:p1332_cli_oom>" "${CMAKE_CURRENT_BINARY_DIR}/p1368-negative" p1368)
add_test(NAME p1368_legacy_C_origin.integration COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1368/compatibility.py" "$<TARGET_FILE:newlangc>"
    "${CMAKE_CURRENT_BINARY_DIR}/p1368-compat")
set_tests_properties(p1368_native.integration p1368_negative_cli_oom.integration
    p1368_legacy_C_origin.integration PROPERTIES TIMEOUT 180)
