# R1213 / C272: independent, immutable V1 source registration only.
# FAST inventories the cases but never executes them.
foreach(_r1213_case IN ITEMS p01 p02 p03 n01 n02 n03 n04 n05 n06)
    if(_r1213_case MATCHES "^p")
        set(_r1213_suffix integration)
    else()
        set(_r1213_suffix unit)
    endif()
    add_test(NAME r1213_c272_${_r1213_case}.${_r1213_suffix}
        COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/r1213/run_case.py"
        "$<TARGET_FILE:newlangc>" "${CMAKE_C_COMPILER}"
        "${NEWLANG_SANITIZER}" "${_r1213_case}"
        "${CMAKE_CURRENT_BINARY_DIR}/r1213-c272")
    set_tests_properties(r1213_c272_${_r1213_case}.${_r1213_suffix}
        PROPERTIES TIMEOUT 240)
    message(STATUS "R1213_C272_REGISTERED_CTEST r1213_c272_${_r1213_case}.${_r1213_suffix}")
endforeach()
