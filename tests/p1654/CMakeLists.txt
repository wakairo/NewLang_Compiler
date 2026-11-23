add_executable(p1654_probe tests/p1654/probe.c)
target_include_directories(p1654_probe PRIVATE src)
target_link_libraries(p1654_probe PRIVATE newlang_checked_c_node newlang_semantic)
target_link_options(p1654_probe PRIVATE -Wl,--wrap=malloc
    -Wl,--wrap=realloc -Wl,--wrap=nl_h_custody_seal_result
    -Wl,--wrap=nl_if_field_prepare -Wl,--wrap=nl_if_field_plan
    -Wl,--wrap=nl_sem_clone -Wl,--wrap=nl_sem_new_value)

set(p1654_source "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1654/selection.nl")
add_test(NAME p1654_custody_rollback.unit COMMAND p1654_probe
    "${p1654_source}" "UINTMAX_C(19)")
set(p1654_discard "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1654/discard.nl")
set(p1658_source "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1658/local-let.nl")
set(p1662_source "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1662/local-let.nl")
set(p1668_divide "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1668/divide.nl")
set(p1668_remainder "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1668/remainder.nl")
add_test(NAME p1654_discard_custody_rollback.unit COMMAND p1654_probe
    "${p1654_discard}" "UINTMAX_C(41)")
add_test(NAME p1658_local_numerator_rollback.unit COMMAND p1654_probe
    "${p1658_source}" "UINTMAX_C(8)")
add_test(NAME p1662_local_remainder_rollback.unit COMMAND p1654_probe
    "${p1662_source}" "UINTMAX_C(5)")
add_test(NAME p1668_joined_divide_rollback.unit COMMAND p1654_probe
    "${p1668_divide}" "UINTMAX_C(5)" "joined")
add_test(NAME p1668_joined_remainder_rollback.unit COMMAND p1654_probe
    "${p1668_remainder}" "UINTMAX_C(5)" "joined")
add_test(NAME p1654_native_scope.unit COMMAND "${Python3_EXECUTABLE}" -B
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1654/run.py"
    "$<TARGET_FILE:newlangc>" "${CMAKE_CURRENT_BINARY_DIR}/p1654-native"
    "${p1654_source}" "${p1654_discard}" "${p1658_source}" "${p1662_source}"
    "${p1668_divide}" "${p1668_remainder}")

foreach(case mixed mixed-reverse)
    add_test(NAME p1677_${case}_rollback.unit COMMAND p1654_probe
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1677/${case}.nl" "UINTMAX_C(8)" "joined")
endforeach()
