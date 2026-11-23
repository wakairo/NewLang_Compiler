add_executable(p1673_probe tests/p1673/probe.c tests/p1673/receipt_mutation.c)
target_link_libraries(p1673_probe PRIVATE newlang_checked_c_node newlang_semantic)
target_link_options(p1673_probe PRIVATE -Wl,--wrap=malloc -Wl,--wrap=calloc
    -Wl,--wrap=realloc -Wl,--wrap=strdup -Wl,--wrap=realpath -Wl,--wrap=fdopen)
set(p1673_source "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1673/source")
foreach(case receipt oom)
    add_test(NAME p1673_${case}.unit COMMAND p1673_probe "${case}"
        "${p1673_source}/product.nl" "${p1673_source}/dependency-expr.nl")
    set_tests_properties(p1673_${case}.unit PROPERTIES TIMEOUT 90)
endforeach()
add_test(NAME p1673_cli_native.integration COMMAND "${Python3_EXECUTABLE}" -B
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1673/run.py" "$<TARGET_FILE:newlangc>"
    "${p1673_source}" "${CMAKE_C_COMPILER}")
set_tests_properties(p1673_cli_native.integration PROPERTIES TIMEOUT 90)
