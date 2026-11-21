add_test(NAME p1605_copy_native.unit COMMAND "${Python3_EXECUTABLE}" -B
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1605_copy/run.py" "$<TARGET_FILE:newlangc>"
    "$<TARGET_FILE:p1605_probe>" "${CMAKE_CURRENT_BINARY_DIR}/p1605-copy-native")
set_tests_properties(p1605_copy_native.unit PROPERTIES TIMEOUT 180)

set(copy_fixture "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1605_copy/source/arm-statement.nl")
add_test(NAME p1605_copy_identity.unit
    COMMAND p1605_probe copy-identity "${copy_fixture}")
add_test(NAME p1605_copy_entry-world-oom.unit
    COMMAND p1605_probe copy-world-oom "${copy_fixture}")
foreach(mode helper-seal-oom c17-staging-oom)
    add_test(NAME p1605_copy_${mode}.unit COMMAND p1605_probe "${mode}"
        "${copy_fixture}")
endforeach()
