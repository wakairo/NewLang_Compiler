foreach(target observe safety oom)
    add_executable(p1217_${target} tests/p1217/${target}.c)
    target_include_directories(p1217_${target} PRIVATE src)
    target_compile_options(p1217_${target} PRIVATE -UNDEBUG)
    target_link_libraries(p1217_${target} PRIVATE newlang_checked_c_node)
endforeach()
target_compile_options(p1217_oom PRIVATE -fno-pie)
target_link_options(p1217_oom PRIVATE -no-pie
    -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc)
add_test(NAME p1217_source.unit COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1217/source_gate_v3.py"
    "$<TARGET_FILE:newlangc>" "${CMAKE_CURRENT_BINARY_DIR}/p1217-source"
    "${CMAKE_C_COMPILER}")
foreach(profile 03-postheap-current-loop 04-preheap-original-current-loop)
    add_test(NAME p1217_${profile}_current.unit COMMAND p1217_observe
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1217/sources/${profile}.nl")
    add_test(NAME p1217_${profile}_safety.unit COMMAND p1217_safety
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1217/sources/${profile}.nl")
endforeach()
add_test(NAME p1217_native.integration COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1217/native_v2.py"
    "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:p1217_observe>"
    "${CMAKE_C_COMPILER}" "${NEWLANG_SANITIZER}"
    "${CMAKE_CURRENT_BINARY_DIR}/p1217-native")
add_test(NAME p1217_oom.unit COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1217/oom_v3.py" "$<TARGET_FILE:p1217_oom>"
    "${CMAKE_CURRENT_BINARY_DIR}/p1217-oom")
set_tests_properties(p1217_oom.unit PROPERTIES RUN_SERIAL TRUE TIMEOUT 300)
add_executable(p1217_resource_status tests/p1217/c274/resource_status.c)
target_include_directories(p1217_resource_status PRIVATE src)
target_compile_options(p1217_resource_status PRIVATE -UNDEBUG -fno-pie)
target_link_options(p1217_resource_status PRIVATE -no-pie
    -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc)
target_link_libraries(p1217_resource_status PRIVATE newlang_checked_c_node)
add_test(NAME p1217_resource_status.unit COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1217/c274/resource_gate_v2.py"
    "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:p1217_resource_status>"
    "$<TARGET_FILE:p1217_oom>" "${CMAKE_C_COMPILER}" "${NEWLANG_SANITIZER}"
    "${CMAKE_CURRENT_BINARY_DIR}/p1217-resource-status")
set_tests_properties(p1217_resource_status.unit PROPERTIES RUN_SERIAL TRUE TIMEOUT 120)


include(tests/p1227/CMakeLists.txt)

include(tests/p1227/c283/CMakeLists.txt)

include(tests/p1236/CMakeLists.txt)
