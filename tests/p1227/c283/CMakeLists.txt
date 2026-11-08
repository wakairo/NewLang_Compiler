foreach(target observe return_safety)
    add_executable(p1227_c283_${target} tests/p1227/c283/${target}.c)
    target_include_directories(p1227_c283_${target} PRIVATE src)
    target_compile_options(p1227_c283_${target} PRIVATE -UNDEBUG)
    target_link_libraries(p1227_c283_${target} PRIVATE newlang_checked_c_node)
endforeach()
add_test(NAME p1227_c283_source_native.integration COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1227/c283/source_native.py"
    "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:p1227_c283_observe>"
    "${CMAKE_C_COMPILER}" "${NEWLANG_SANITIZER}"
    "${CMAKE_CURRENT_BINARY_DIR}/p1227-c283-source-native")
set_tests_properties(p1227_c283_source_native.integration PROPERTIES RUN_SERIAL TRUE TIMEOUT 120)
foreach(profile A-alias-preH A-alias-postH)
    add_test(NAME p1227_c283_${profile}_return_safety.unit COMMAND p1227_c283_return_safety
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1227/c283/sources/${profile}.nl")
endforeach()
newlang_add_legacy_audit(NAME p1227_c283_history_negative.unit COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1236/historical_v61_negative.py")
if(NEWLANG_LEGACY_AUDIT)
    set_tests_properties(p1227_c283_history_negative.unit PROPERTIES RUN_SERIAL TRUE TIMEOUT 120)
endif()
add_test(NAME p1227_c283_alias_oom.unit COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1227/c283/alias_oom.py"
    "$<TARGET_FILE:p1227_oom>" "${CMAKE_CURRENT_BINARY_DIR}/p1227-oom"
    "${CMAKE_CURRENT_BINARY_DIR}/p1227-c283-alias-oom")
set_tests_properties(p1227_c283_alias_oom.unit PROPERTIES
    DEPENDS p1227_oom.unit RUN_SERIAL TRUE TIMEOUT 120)
