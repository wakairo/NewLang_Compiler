foreach(target roles safety oom)
    add_executable(p1208_${target} tests/p1208/${target}.c)
    target_include_directories(p1208_${target} PRIVATE src)
    target_compile_options(p1208_${target} PRIVATE -UNDEBUG)
    target_link_libraries(p1208_${target} PRIVATE
        newlang_semantic newlang_parser newlang_checked_c_node)
endforeach()
add_test(NAME p1208_negative.unit COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1208/negative.py" "$<TARGET_FILE:newlangc>")
add_test(NAME p1208_native.integration COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1208/c266/native.py" "$<TARGET_FILE:newlangc>"
    "$<TARGET_FILE:p1208_roles>" "$<TARGET_FILE:p1193_roles>"
    "${CMAKE_C_COMPILER}" "${NEWLANG_SANITIZER}"
    "${CMAKE_CURRENT_BINARY_DIR}/p1208-native")
add_test(NAME p1208_evidence.unit COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1208/c266/inputs.py")
add_test(NAME p1208_combined_source.unit COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1208/c266/source_gate.py"
    "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:p1208_roles>"
    "$<TARGET_FILE:p1193_roles>" "$<TARGET_FILE:p1193_census>"
    "${CMAKE_CURRENT_BINARY_DIR}/p1208-combined-source")
add_test(NAME p1208_oom.unit COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1208/c266/oom.py" "$<TARGET_FILE:p1208_oom>"
    "${CMAKE_CURRENT_BINARY_DIR}/p1208-oom")
set_tests_properties(p1208_oom.unit PROPERTIES RUN_SERIAL TRUE TIMEOUT 300)
target_compile_options(p1208_oom PRIVATE -fno-pie)
target_link_options(p1208_oom PRIVATE -no-pie
    -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc)
foreach(profile 01-left-normal 02-right-normal 03-left-final-return
        04-right-final-return 05-left-discard-normal
        80-normal-control 81-mixed-tail-control 82-both-return-control
        83-normal-final-return-control)
    add_test(NAME p1208_${profile}_current.unit COMMAND p1208_roles
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1208/sources/${profile}.nl")
endforeach()
foreach(profile 03-left-final-return 04-right-final-return 05-left-discard-normal)
    add_test(NAME p1208_${profile}_safety.unit COMMAND p1208_safety
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1208/sources/${profile}.nl")
endforeach()
