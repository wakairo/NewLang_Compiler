# C280 Phase1 source-owned current/RETURN consumer; no public issuer or ABI.
foreach(target observe safety oom)
    add_executable(p1227_${target} tests/p1227/${target}.c)
    target_include_directories(p1227_${target} PRIVATE src)
    target_compile_options(p1227_${target} PRIVATE -UNDEBUG)
    target_link_libraries(p1227_${target} PRIVATE newlang_checked_c_node)
endforeach()
target_compile_options(p1227_oom PRIVATE -fno-pie)
target_link_options(p1227_oom PRIVATE -no-pie
    -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc)
add_test(NAME p1227_source_native.integration COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1227/source_native_v2.py"
    "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:p1227_observe>"
    "${CMAKE_C_COMPILER}" "${NEWLANG_SANITIZER}"
    "${CMAKE_CURRENT_BINARY_DIR}/p1227-source-native")
set_tests_properties(p1227_source_native.integration PROPERTIES TIMEOUT 120 RUN_SERIAL TRUE)
foreach(profile A-preH A-postH A-inverse-preH A-inverse-postH)
    add_test(NAME p1227_${profile}_safety.unit COMMAND p1227_safety
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1227/sources/${profile}.nl")
endforeach()
add_test(NAME p1227_oom.unit COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1227/oom_gate.py" "$<TARGET_FILE:p1227_oom>"
    "${CMAKE_CURRENT_BINARY_DIR}/p1227-oom")
set_tests_properties(p1227_oom.unit PROPERTIES RUN_SERIAL TRUE TIMEOUT 300)
