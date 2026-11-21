# R1589 original FIRST evidence only. Never merge this R branch.
add_executable(r1589_public_probe tests/r1589/probe.c)
target_link_libraries(r1589_public_probe PRIVATE newlang_semantic)

set(r1589_cases
    positive_parcel
    positive_receipt
    invalid_condition
    invalid_arm_type
    invalid_zero_divisor
    lawful_no_if_control)
foreach(r1589_case IN LISTS r1589_cases)
    add_test(NAME "r1589.original_${r1589_case}"
        COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/r1589/driver.py" measure
        "${r1589_case}" "$<TARGET_FILE:newlangc>"
        "$<TARGET_FILE:r1589_public_probe>" "${CMAKE_C_COMPILER}"
        "${NEWLANG_SANITIZER}" "${CMAKE_BUILD_TYPE}"
        "${CMAKE_CURRENT_BINARY_DIR}/r1589-evidence")
endforeach()

# Print the actual complete inventory during the existing build. This runs
# zero CTests and no guest compiler operation. It supplies the names that the
# unchanged FAST workflow otherwise redirects into a runner-local file.
add_custom_target(r1589_inventory ALL
    COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/r1589/driver.py" inventory
        "${CMAKE_CTEST_COMMAND}" "${CMAKE_CURRENT_BINARY_DIR}"
    DEPENDS r1589_public_probe newlangc
    VERBATIM)
