add_executable(p1517_public_owned tests/p1517/public_owned.c)
target_link_libraries(p1517_public_owned PRIVATE newlang_semantic newlang_parser)
add_executable(p1517_safety tests/p1517/safety.c)
target_include_directories(p1517_safety PRIVATE src)
target_link_libraries(p1517_safety PRIVATE newlang_checked_c_node newlang_semantic newlang_parser)
target_link_options(p1517_safety PRIVATE -Wl,--wrap=malloc -Wl,--wrap=realloc -Wl,--wrap=nl_sem_clone)
add_executable(p1517_cli_fault src/main.c tests/p1517/cli_fault.c)
target_link_libraries(p1517_cli_fault PRIVATE newlang_checked_c_node newlang_semantic newlang_diagnostic)
target_link_options(p1517_cli_fault PRIVATE -Wl,--wrap=malloc)
foreach(variant zero nonzero)
    if(variant STREQUAL "zero")
        set(p1517_sample 0)
    else()
        set(p1517_sample 42)
    endif()
    add_test(NAME p1517_${variant}_owned.unit COMMAND p1517_public_owned
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1517/P-${variant}.nl" ${p1517_sample})
endforeach()
add_test(NAME p1517_owner_authentication.unit COMMAND p1517_safety attack
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1517/P-zero.nl")
foreach(point RANGE 0 5)
    add_test(NAME p1517_oom_${point}.unit COMMAND p1517_safety oom
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1517/P-zero.nl" ${point})
endforeach()
foreach(case H1-leak foreign-allocation foreign-domain foreign-pointer
             incomplete-callee owner-reused stale-read)
    add_test(NAME p1517_negative_${case}.unit COMMAND p1517_safety negative
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1517/N-${case}.nl")
endforeach()
add_test(NAME p1517_native.integration COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1517/native.py" "$<TARGET_FILE:newlangc>"
    "${CMAKE_C_COMPILER}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1517"
    "${CMAKE_CURRENT_BINARY_DIR}/p1517-native" "${NEWLANG_SANITIZER}")
add_test(NAME p1517_named_no_clobber.integration COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1517/named.py" "$<TARGET_FILE:newlangc>"
    "$<TARGET_FILE:p1517_cli_fault>" "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1517"
    "${CMAKE_CURRENT_BINARY_DIR}/p1517-named")
set_tests_properties(p1517_native.integration p1517_named_no_clobber.integration
    PROPERTIES TIMEOUT 180 RUN_SERIAL TRUE)
