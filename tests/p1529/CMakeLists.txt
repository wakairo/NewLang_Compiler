add_executable(p1529_public_owned tests/p1529/public_owned.c)
target_link_libraries(p1529_public_owned PRIVATE newlang_semantic newlang_parser)
add_executable(p1529_consumer tests/p1529/consumer.c)
target_include_directories(p1529_consumer PRIVATE src)
target_link_libraries(p1529_consumer PRIVATE newlang_checked_c_node newlang_semantic newlang_parser)
target_link_options(p1529_consumer PRIVATE -Wl,--wrap=malloc -Wl,--wrap=realloc -Wl,--wrap=nl_sem_clone)
foreach(p1529_variant A-prefix-zero B-prefix-nonzero C-oldcopy-zero D-combined-nonzero)
    if(p1529_variant MATCHES "nonzero")
        set(p1529_sample 37)
    else()
        set(p1529_sample 0)
    endif()
    add_test(NAME p1529_${p1529_variant}_owned.unit COMMAND p1529_public_owned
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1529/${p1529_variant}.nl" ${p1529_sample})
endforeach()
add_test(NAME p1529_prefix_authentication.unit COMMAND p1529_consumer attack
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1529/D-combined-nonzero.nl")
foreach(p1529_negative foreign-allocation foreign-domain foreign-pointer live-loan)
    add_test(NAME p1529_negative_${p1529_negative}.unit COMMAND p1517_safety negative
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1529/N-${p1529_negative}.nl")
endforeach()
add_test(NAME p1529_native.integration COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1529/native.py" "$<TARGET_FILE:newlangc>"
    "${CMAKE_C_COMPILER}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1529"
    "${CMAKE_CURRENT_BINARY_DIR}/p1529-native" "${NEWLANG_SANITIZER}")
set_tests_properties(p1529_native.integration PROPERTIES TIMEOUT 180 RUN_SERIAL TRUE)
