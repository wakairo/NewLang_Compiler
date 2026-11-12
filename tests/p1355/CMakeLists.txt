add_executable(p1355_wrapper tests/p1355/owned.c)
target_include_directories(p1355_wrapper PRIVATE src)
target_compile_options(p1355_wrapper PRIVATE -UNDEBUG)
target_link_libraries(p1355_wrapper PRIVATE newlang_checked_c_node)
target_link_options(p1355_wrapper PRIVATE
    -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc
    -Wl,--wrap=nl_copy_body_current -Wl,--wrap=nl_h_custody_seal_result
    -Wl,--wrap=nl_copy_body_event -Wl,--wrap=nl_copy_body_seal
    -Wl,--wrap=nl_copy_body_readonly)
foreach(profile A-input83 B-input137-index1)
    if(profile STREQUAL "A-input83")
        set(index 0)
    else()
        set(index 1)
    endif()
    foreach(mode positive oom)
        add_test(NAME p1355_${profile}_${mode}.unit COMMAND p1355_wrapper ${mode}
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1355/${profile}.nl" ${index})
        set_tests_properties(p1355_${profile}_${mode}.unit PROPERTIES TIMEOUT 180)
    endforeach()
endforeach()
add_test(NAME p1355_native.integration COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1355/native.py" "$<TARGET_FILE:newlangc>"
    "${CMAKE_C_COMPILER}" "${CMAKE_CURRENT_BINARY_DIR}/p1355-native"
    "${NEWLANG_SANITIZER}")
add_test(NAME p1355_negative_cli_oom.integration COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1355/failure.py" "$<TARGET_FILE:newlangc>"
    "$<TARGET_FILE:p1332_cli_oom>" "${CMAKE_CURRENT_BINARY_DIR}/p1355-negative" p1355)
set_tests_properties(p1355_native.integration p1355_negative_cli_oom.integration
    PROPERTIES TIMEOUT 180)
add_test(NAME p1355_semantic_negative.unit COMMAND p1355_wrapper negative
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1355/N-ended-loan.nl"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1355/N-wrong-argument.nl")
foreach(profile N-two-inner N-extra-wrapper)
    add_test(NAME p1355_${profile}.unit COMMAND p1355_wrapper precision
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1355/${profile}.nl")
endforeach()

add_test(NAME p1355_legacy_C_origin.integration COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1355/compatibility.py" "$<TARGET_FILE:newlangc>"
    "${CMAKE_CURRENT_BINARY_DIR}/p1355-compat")
set_tests_properties(p1355_legacy_C_origin.integration PROPERTIES TIMEOUT 180)
