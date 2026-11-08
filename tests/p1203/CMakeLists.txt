add_executable(p1203_safety tests/p1203/safety.c)
newlang_add_legacy_audit(NAME p1203_evidence.unit COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1208/c266/p1203_evidence_current.py")
target_include_directories(p1203_safety PRIVATE src)
target_compile_options(p1203_safety PRIVATE -UNDEBUG)
target_link_libraries(p1203_safety PRIVATE
    newlang_semantic newlang_parser newlang_checked_c_node)
foreach(profile 01-many 02-zero 03-one 04-second-field 05-return-helper 80-phase-control)
    add_test(NAME p1203_${profile}_current.unit COMMAND p1193_roles
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1203/sources/${profile}.nl")
endforeach()
foreach(profile 01-many 05-return-helper)
    add_test(NAME p1203_${profile}_safety.unit COMMAND p1203_safety
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1203/sources/${profile}.nl")
endforeach()
add_test(NAME p1203_census.unit COMMAND p1193_census
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1203/sources/01-many.nl")
add_test(NAME p1203_oom.unit COMMAND p1193_oom
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1203/sources/01-many.nl")
set_tests_properties(p1203_oom.unit PROPERTIES RUN_SERIAL TRUE TIMEOUT 300)
add_test(NAME p1203_negative.unit COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1203/negative.py" "$<TARGET_FILE:newlangc>")
add_test(NAME p1203_native.integration COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1203/native.py"
    "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:p1193_roles>"
    "${CMAKE_C_COMPILER}" "${NEWLANG_SANITIZER}"
    "${CMAKE_CURRENT_BINARY_DIR}/p1203-native")
