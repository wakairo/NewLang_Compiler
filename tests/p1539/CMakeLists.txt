add_executable(p1539_public_owned tests/p1539/public_owned.c)
target_link_libraries(p1539_public_owned PRIVATE newlang_semantic newlang_parser)
foreach(variant zero nonzero)
    if(variant STREQUAL "zero")
        set(p1539_arguments 0 91 23)
    else()
        set(p1539_arguments 37 83 27)
    endif()
    add_test(NAME p1539_${variant}_owned.unit COMMAND p1539_public_owned
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1539/P-${variant}.nl" ${p1539_arguments})
endforeach()
foreach(case foreign-allocation foreign-domain live-read-at-end post-end-read)
    add_test(NAME p1539_negative_${case}.unit COMMAND p1517_safety negative
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1539/N-${case}.nl")
endforeach()
add_test(NAME p1539_artifact_authentication.unit COMMAND p1529_consumer attack
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1539/P-nonzero.nl")
foreach(site 4 5)
    add_test(NAME p1539_rollback_${site}.unit COMMAND p1517_safety oom
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1539/P-nonzero.nl" ${site})
endforeach()
add_test(NAME p1539_native.integration COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1539/native.py" "$<TARGET_FILE:newlangc>"
    "${CMAKE_C_COMPILER}" "$<TARGET_FILE:p1529_consumer>"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1539"
    "${CMAKE_CURRENT_BINARY_DIR}/p1539-native" "${NEWLANG_SANITIZER}")
set_tests_properties(p1539_native.integration PROPERTIES TIMEOUT 180 RUN_SERIAL TRUE)
