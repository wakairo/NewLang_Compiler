# R1478: independent original FIRST, registration only on R-FAST.
add_executable(r1478_public_probe tests/r1478/probe.c)
target_include_directories(r1478_public_probe PRIVATE src)
target_link_libraries(r1478_public_probe PRIVATE
    newlang_checked_c_node newlang_semantic newlang_parser)

foreach(r1478_case s1 s2 s3)
    add_test(NAME r1478_${r1478_case}_original.integration
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/r1478/runner.py"
            "${r1478_case}" "$<TARGET_FILE:newlangc>"
            "$<TARGET_FILE:r1478_public_probe>"
            "${CMAKE_C_COMPILER}" "${NEWLANG_SANITIZER}"
            "${CMAKE_CURRENT_BINARY_DIR}/r1478-native")
    set_tests_properties(r1478_${r1478_case}_original.integration
        PROPERTIES TIMEOUT 180 SKIP_RETURN_CODE 125)
endforeach()
