add_executable(p1451_probe tests/p1451/probe.c)
target_link_libraries(p1451_probe PRIVATE newlang_checked_c_node
    newlang_semantic newlang_parser)
target_link_options(p1451_probe PRIVATE -Wl,--wrap=malloc -Wl,--wrap=realloc
    -Wl,--wrap=nl_sem_clone)
set(p1451_base "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1451")
foreach(shape A-front-install B-back-alias-install L-literal-install)
    foreach(mode observe emit)
        add_test(NAME p1451_${shape}_${mode}.unit COMMAND p1451_probe ${mode}
            "${p1451_base}/${shape}.nl")
    endforeach()
endforeach()
add_test(NAME p1451_call_auth.unit COMMAND p1451_probe auth
    "${p1451_base}/A-front-install.nl" "${p1451_base}/B-back-alias-install.nl")
add_test(NAME p1451_domain_rollback.unit COMMAND p1451_probe negative
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1441/N-wrong-domain.nl"
    "${p1451_base}/A-front-install.nl" ALLOCATED-DOMAIN-MISMATCH)
add_test(NAME p1451_loan_rollback.unit COMMAND p1451_probe negative
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1441/N-loan-escape.nl"
    "${p1451_base}/A-front-install.nl" P8-EXIT-DEPENDENCY)
foreach(point 1 3)
    add_test(NAME p1451_oom_${point}.unit COMMAND p1451_probe oom${point}
        "${p1451_base}/A-front-install.nl")
endforeach()
add_test(NAME p1451_cli_native.integration COMMAND "${Python3_EXECUTABLE}"
    "${p1451_base}/native.py" "$<TARGET_FILE:newlangc>"
    "${CMAKE_C_COMPILER}" "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_BINARY_DIR}/p1451-native")
set_tests_properties(p1451_cli_native.integration PROPERTIES TIMEOUT 180)
