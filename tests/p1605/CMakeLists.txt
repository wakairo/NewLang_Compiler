add_executable(p1605_probe tests/p1605/probe.c)
target_include_directories(p1605_probe PRIVATE src)
target_link_libraries(p1605_probe PRIVATE newlang_checked_c_node newlang_semantic)
target_link_options(p1605_probe PRIVATE
    -Wl,--wrap=malloc -Wl,--wrap=realloc
    -Wl,--wrap=nl_h_custody_reserve -Wl,--wrap=nl_h_custody_seal_result)

add_test(NAME p1605_if_arm_native.unit COMMAND "${Python3_EXECUTABLE}" -B
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1605/run.py" "$<TARGET_FILE:newlangc>"
    "$<TARGET_FILE:p1605_probe>" "${CMAKE_CURRENT_BINARY_DIR}/p1605-native")
set_tests_properties(p1605_if_arm_native.unit PROPERTIES TIMEOUT 180)

add_test(NAME p1605_if_arm_identity.unit COMMAND p1605_probe identity
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1605/source/arm-copy-unknown.nl")
foreach(mode arm-reservation-oom helper-seal-oom arm-world-oom c17-staging-oom)
    if(mode STREQUAL "arm-world-oom")
        set(fixture arm-main)
    else()
        set(fixture arm-dispatch)
    endif()
    add_test(NAME p1605_${mode}.unit COMMAND p1605_probe "${mode}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1605/source/${fixture}.nl")
endforeach()
