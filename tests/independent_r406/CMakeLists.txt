if(BUILD_TESTING)
    add_test(NAME r406_fresh_blind_native.integration
        COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/independent_r406/fresh_blind.py"
        "$<TARGET_FILE:newlangc>" "${CMAKE_C_COMPILER}" "${NEWLANG_SANITIZER}")
    set_tests_properties(r406_fresh_blind_native.integration PROPERTIES
        TIMEOUT 180 ENVIRONMENT
        "PYTHONDONTWRITEBYTECODE=1;PYTHONHASHSEED=0;LC_ALL=C;ASAN_OPTIONS=detect_leaks=1:halt_on_error=1;UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1")
endif()
