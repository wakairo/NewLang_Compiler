# R-owned registered integration; normal compiler source remains untouched.
if(BUILD_TESTING)
    add_executable(r653_owned_probe tests/r653/owned_probe.c)
    target_link_libraries(r653_owned_probe PRIVATE newlang_checked_c_node newlang_semantic)
    add_test(NAME r653_fresh_blind_owned_and_native.integration
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/r653/native_test.py"
        "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:r653_owned_probe>"
        "${CMAKE_C_COMPILER}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/r653"
        "${NEWLANG_SANITIZER}")
endif()
