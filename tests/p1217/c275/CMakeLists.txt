# CURRENT successors retain the original targets/source for historical diagnosis.
file(SHA256 "${CMAKE_CURRENT_SOURCE_DIR}/tests/unit/live_return_backend_test.c" c275_original_unit)
if(NOT c275_original_unit STREQUAL "d87cf2a04938687c4774db5484a12f5c540fcd1096f3e3f32c99802a0a0427b6")
    message(FATAL_ERROR "C275 original H backend contract changed")
endif()
file(SHA256 "${CMAKE_CURRENT_SOURCE_DIR}/tests/p733/gate_test.c" c275_original_p733)
if(NOT c275_original_p733 STREQUAL "89a74a19a907acb29629adf7bf63013c7c027d4eb2d3a467b4ac893435382eea")
    message(FATAL_ERROR "C275 original local-return contract changed")
endif()
foreach(contract live_return_backend p733)
    add_executable(p1217_c275_${contract}_current
        tests/p1217/c275/${contract}_current_v1.c)
    target_compile_options(p1217_c275_${contract}_current PRIVATE -fno-pie)
    target_link_options(p1217_c275_${contract}_current PRIVATE -no-pie -Wl,--wrap=malloc)
    target_link_libraries(p1217_c275_${contract}_current PRIVATE
        newlang_semantic newlang_parser newlang_checked_c_node)
endforeach()
target_link_options(p1217_c275_p733_current PRIVATE
    -Wl,--wrap=calloc -Wl,--wrap=realloc)
add_test(NAME p1217_c275_historical_contract.unit COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1217/c275/history_test.py")
