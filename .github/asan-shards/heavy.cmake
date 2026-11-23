# Preserve the ordinary registration and relax only the two approved Heavy tests.
if(NOT PROJECT_NAME STREQUAL "NewLangCompiler")
    return()
endif()
include("${CMAKE_SOURCE_DIR}/cmake/independent-r-tests-hook.cmake")

function(_newlang_asan_heavy_overlap)
    if(NOT "$ENV{NEWLANG_CI_ASAN_SHARD}" STREQUAL "heavy" OR
       NOT NEWLANG_SANITIZER STREQUAL "address" OR
       NOT CMAKE_BUILD_TYPE STREQUAL "Debug" OR NEWLANG_FRONTEND_ONLY)
        message(FATAL_ERROR "Heavy overlap requires the full Debug ASan Heavy configuration")
    endif()
    foreach(test_name p1291_named_oom.integration p1299_named_oom.integration)
        if(NOT TEST "${test_name}")
            message(FATAL_ERROR "Missing approved Heavy test ${test_name}")
        endif()
        get_property(serial TEST "${test_name}" PROPERTY RUN_SERIAL)
        if(NOT serial)
            message(FATAL_ERROR "Missing original RUN_SERIAL for ${test_name}")
        endif()
        set_property(TEST "${test_name}" PROPERTY RUN_SERIAL)
    endforeach()
endfunction()

function(_newlang_asan_defer_heavy_overlap)
    cmake_language(DEFER CALL _newlang_asan_heavy_overlap)
endfunction()
cmake_language(DEFER CALL _newlang_asan_defer_heavy_overlap)
