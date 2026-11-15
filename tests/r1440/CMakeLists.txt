# C958: same original full files; public driver never executes guest/native.
add_executable(r1440_probe tests/r1440/probe.c)
target_link_libraries(r1440_probe PRIVATE
    newlang_checked_c_node newlang_semantic newlang_parser)

foreach(_r1440_case IN ITEMS R1 R2 R3)
    add_test(NAME "r1440_${_r1440_case}_first_reach.unit"
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/r1440/run_case.py"
            "$<TARGET_FILE:r1440_probe>"
            "${_r1440_case}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/r1440/${_r1440_case}.nl")
endforeach()
