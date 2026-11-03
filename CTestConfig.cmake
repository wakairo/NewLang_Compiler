# R1120 C219: additive standard CTest registration; existing files stay fixed.
if(PROJECT_NAME STREQUAL "NewLangCompiler" AND BUILD_TESTING)
    function(_r1120_c219_deferred_registration)
        include("${CMAKE_SOURCE_DIR}/tests/redteam/r1120_c219/registration.cmake")
    endfunction()
    cmake_language(DEFER CALL _r1120_c219_deferred_registration)
endif()
