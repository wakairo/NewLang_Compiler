add_executable(p1375_owned tests/p1375/owned.c)
target_include_directories(p1375_owned PRIVATE src)
target_compile_options(p1375_owned PRIVATE -UNDEBUG)
target_link_libraries(p1375_owned PRIVATE newlang_checked_c_node)
target_link_options(p1375_owned PRIVATE
    -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc
    -Wl,--wrap=nl_sem_clone -Wl,--wrap=nl_body_create_span
    -Wl,--wrap=nl_h_custody_seal_result)
foreach(profile D-nonidentity-old-copy E-index-zero)
    if(profile STREQUAL "D-nonidentity-old-copy")
        set(index 1)
    else()
        set(index 0)
    endif()
    foreach(mode positive registration-oom oom copy-oom)
        add_test(NAME p1375_${profile}_${mode}.unit COMMAND p1375_owned ${mode}
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1375/${profile}.nl" ${index})
        set_tests_properties(p1375_${profile}_${mode}.unit PROPERTIES TIMEOUT 180)
    endforeach()
endforeach()
add_test(NAME p1375_loan_negative.unit COMMAND p1375_owned negative-loan
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1375/N-loan-escape.nl" 1)
add_test(NAME p1375_profile_negative.unit COMMAND p1375_owned negative-profile
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1375/N-return-formal.nl" 1)
add_test(NAME p1375_type_negative.unit COMMAND p1375_owned negative-type
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1375/N-wrong-actual.nl" 1)
add_test(NAME p1375_native.integration COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1375/native.py" "$<TARGET_FILE:newlangc>"
    "${CMAKE_C_COMPILER}" "${CMAKE_CURRENT_BINARY_DIR}/p1375-native"
    "${NEWLANG_SANITIZER}")
add_test(NAME p1375_negative_cli_oom.integration COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1375/failure.py" "$<TARGET_FILE:newlangc>"
    "$<TARGET_FILE:p1332_cli_oom>" "${CMAKE_CURRENT_BINARY_DIR}/p1375-negative" p1375)
add_test(NAME p1375_legacy_C_origin.integration COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1375/compatibility.py" "$<TARGET_FILE:newlangc>"
    "${CMAKE_CURRENT_BINARY_DIR}/p1375-compat")
set_tests_properties(p1375_native.integration p1375_negative_cli_oom.integration
    p1375_legacy_C_origin.integration PROPERTIES TIMEOUT 180)
