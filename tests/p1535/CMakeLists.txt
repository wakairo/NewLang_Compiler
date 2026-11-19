add_executable(p1535_public_owned tests/p1535/public_owned.c)
target_link_libraries(p1535_public_owned PRIVATE newlang_semantic newlang_parser)
foreach(variant zero nonzero composed)
    if(variant STREQUAL "zero")
        set(p1535_arguments 0 91 23)
    elseif(variant STREQUAL "nonzero")
        set(p1535_arguments 42 91 23)
    else()
        set(p1535_arguments 37 83 27)
    endif()
    add_test(NAME p1535_${variant}_owned.unit COMMAND p1535_public_owned
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1535/P-${variant}.nl" ${p1535_arguments})
endforeach()
foreach(case foreign-allocation foreign-domain foreign-pointer live-read-at-end post-end-read)
    add_test(NAME p1535_negative_${case}.unit COMMAND p1517_safety negative
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1535/N-${case}.nl")
endforeach()
add_test(NAME p1535_owner_authentication.unit COMMAND p1517_safety attack
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1535/P-zero.nl")
add_test(NAME p1535_native.integration COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1535/native.py" "$<TARGET_FILE:newlangc>"
    "${CMAKE_C_COMPILER}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1535"
    "${CMAKE_CURRENT_BINARY_DIR}/p1535-native" "${NEWLANG_SANITIZER}")
set_tests_properties(p1535_native.integration PROPERTIES TIMEOUT 180 RUN_SERIAL TRUE)
