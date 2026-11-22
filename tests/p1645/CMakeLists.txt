add_executable(p1645_probe tests/p1645/probe.c)
target_include_directories(p1645_probe PRIVATE src)
target_link_libraries(p1645_probe PRIVATE newlang_checked_c_node newlang_semantic)
target_link_options(p1645_probe PRIVATE -Wl,--wrap=malloc
    -Wl,--wrap=nl_h_custody_seal_result)

set(p1645_source "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1645/source")
add_test(NAME p1645_identity.unit COMMAND p1645_probe identity
    "${p1645_source}/arm-both.nl")
add_test(NAME p1645_rollback.unit COMMAND p1645_probe rollback
    "${p1645_source}/arm-direct.nl")
foreach(mode native scope)
    add_test(NAME p1645_${mode}.unit COMMAND "${Python3_EXECUTABLE}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1645/run.py" ${mode}
        "$<TARGET_FILE:newlangc>" "${CMAKE_CURRENT_BINARY_DIR}/p1645-${mode}"
        "${p1645_source}")
    set_tests_properties(p1645_${mode}.unit PROPERTIES TIMEOUT 180)
endforeach()
