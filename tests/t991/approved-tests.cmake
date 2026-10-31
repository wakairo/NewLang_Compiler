# T991 reviewed synthetic registration only. Never load an arbitrary R-owned path.
if(NOT EXISTS "${CMAKE_SOURCE_DIR}/tests/t991/runner.py")
    message(FATAL_ERROR "T991: approved runner missing")
endif()
foreach(_mode accept reject)
    add_test(NAME t991_contract_${_mode}.integration COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/tests/t991/runner.py" "${_mode}")
    set_tests_properties(t991_contract_${_mode}.integration PROPERTIES TIMEOUT 90
        ENVIRONMENT "PYTHONDONTWRITEBYTECODE=1;PYTHONHASHSEED=0;LC_ALL=C")
endforeach()
