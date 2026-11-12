add_executable(p1364_wrapper tests/p1364/owned.c)
target_include_directories(p1364_wrapper PRIVATE src)
target_compile_options(p1364_wrapper PRIVATE -UNDEBUG)
target_link_libraries(p1364_wrapper PRIVATE newlang_checked_c_node)
target_link_options(p1364_wrapper PRIVATE
    -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc
    -Wl,--wrap=nl_copy_body_current -Wl,--wrap=nl_h_custody_seal_result
    -Wl,--wrap=nl_copy_body_event -Wl,--wrap=nl_copy_body_seal
    -Wl,--wrap=nl_copy_body_readonly -Wl,--wrap=nl_sem_clone
    -Wl,--wrap=nl_body_create_span)
foreach(profile A-caller-original B-reversed-field)
    if(profile STREQUAL "A-caller-original")
        set(index 1)
    else()
        set(index 0)
    endif()
    foreach(mode positive registration-oom oom)
        add_test(NAME p1364_${profile}_${mode}.unit COMMAND p1364_wrapper ${mode}
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1364/${profile}.nl" ${index})
        set_tests_properties(p1364_${profile}_${mode}.unit PROPERTIES TIMEOUT 180)
    endforeach()
endforeach()
add_test(NAME p1364_semantic_negative.unit COMMAND p1364_wrapper negative
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1364/N-ended-loan.nl"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1364/N-wrong-argument.nl")
foreach(profile N-extra-main N-second-wrapper)
    add_test(NAME p1364_${profile}.unit COMMAND p1364_wrapper precision
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1364/${profile}.nl")
endforeach()
add_test(NAME p1364_native.integration COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1364/native.py" "$<TARGET_FILE:newlangc>"
    "${CMAKE_C_COMPILER}" "${CMAKE_CURRENT_BINARY_DIR}/p1364-native"
    "${NEWLANG_SANITIZER}")
add_test(NAME p1364_negative_cli_oom.integration COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1364/failure.py" "$<TARGET_FILE:newlangc>"
    "$<TARGET_FILE:p1332_cli_oom>" "${CMAKE_CURRENT_BINARY_DIR}/p1364-negative" p1364)
add_test(NAME p1364_legacy_C_origin.integration COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1364/compatibility.py" "$<TARGET_FILE:newlangc>"
    "${CMAKE_CURRENT_BINARY_DIR}/p1364-compat")
set_tests_properties(p1364_native.integration p1364_negative_cli_oom.integration
    p1364_legacy_C_origin.integration PROPERTIES TIMEOUT 180)
