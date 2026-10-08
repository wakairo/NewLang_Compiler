cmake_minimum_required(VERSION 3.28)
project(NewLangCompiler VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_C_COMPILER_ID MATCHES "^(GNU|Clang)$")
    message(FATAL_ERROR "P0 validates GCC and Clang host compilers only")
endif()
if(CMAKE_VERSION VERSION_GREATER_EQUAL 4.0)
    message(FATAL_ERROR "P0 CMake policy is >=3.28,<4.0")
endif()

set(NEWLANG_SANITIZER "none" CACHE STRING "none, address, or undefined")
set_property(CACHE NEWLANG_SANITIZER PROPERTY STRINGS none address undefined)
add_library(newlang_options INTERFACE)
target_compile_options(newlang_options INTERFACE
    -Wall -Wextra -Wpedantic -Werror)
if(NOT NEWLANG_SANITIZER STREQUAL "none")
    if(NOT NEWLANG_SANITIZER MATCHES "^(address|undefined)$")
        message(FATAL_ERROR "Unsupported NEWLANG_SANITIZER=${NEWLANG_SANITIZER}")
    endif()
    target_compile_options(newlang_options INTERFACE
        -fsanitize=${NEWLANG_SANITIZER} -fno-omit-frame-pointer
        -fno-sanitize-recover=all)
    target_link_options(newlang_options INTERFACE
        -fsanitize=${NEWLANG_SANITIZER} -fno-sanitize-recover=all)
endif()

find_program(LLVM_CONFIG NAMES llvm-config-23 REQUIRED)
execute_process(COMMAND "${LLVM_CONFIG}" --version
    OUTPUT_VARIABLE llvm_version OUTPUT_STRIP_TRAILING_WHITESPACE
    COMMAND_ERROR_IS_FATAL ANY)
if(NOT llvm_version STREQUAL "23.1.2")
    message(FATAL_ERROR "LLVM must be exactly 23.1.2; found ${llvm_version}")
endif()
execute_process(COMMAND "${LLVM_CONFIG}" --includedir
    OUTPUT_VARIABLE llvm_include OUTPUT_STRIP_TRAILING_WHITESPACE
    COMMAND_ERROR_IS_FATAL ANY)
execute_process(COMMAND "${LLVM_CONFIG}" --libdir
    OUTPUT_VARIABLE llvm_libdir OUTPUT_STRIP_TRAILING_WHITESPACE
    COMMAND_ERROR_IS_FATAL ANY)
find_library(llvm_shared NAMES LLVM-23 LLVM
    PATHS "${llvm_libdir}" NO_DEFAULT_PATH REQUIRED)
add_library(newlang_llvm SHARED IMPORTED)
set_target_properties(newlang_llvm PROPERTIES
    IMPORTED_LOCATION "${llvm_shared}"
    INTERFACE_INCLUDE_DIRECTORIES "${llvm_include}")
message(STATUS "LLVM C API ${llvm_version}: ${llvm_shared}")

add_library(newlang_diagnostic STATIC src/diagnostic.c)
target_include_directories(newlang_diagnostic PUBLIC include)
target_link_libraries(newlang_diagnostic PUBLIC newlang_options)
add_library(newlang_source STATIC src/source.c)
target_include_directories(newlang_source PUBLIC include)
target_link_libraries(newlang_source PUBLIC newlang_options)
add_library(newlang_lexer STATIC src/lexer.c)
target_link_libraries(newlang_lexer PUBLIC newlang_source)
add_library(newlang_syntax STATIC src/syntax.c)
target_link_libraries(newlang_syntax PUBLIC newlang_source)
add_library(newlang_parser STATIC src/parser.c)
target_link_libraries(newlang_parser PUBLIC newlang_syntax newlang_lexer newlang_diagnostic)
add_library(newlang_checked STATIC src/checked.c)
target_link_libraries(newlang_checked PUBLIC newlang_syntax)
add_library(newlang_semantic STATIC src/semantic.c src/semantic_check.c src/recursive_type.c src/allocated_node.c src/allocated_join.c src/fixed_field.c src/sum.c src/raw_storage.c src/function_body.c src/typed_owner.c src/control.c)
target_link_libraries(newlang_semantic PUBLIC newlang_checked newlang_diagnostic newlang_parser)
add_library(newlang_checked_c_node STATIC src/checked_c_node.c)
target_link_libraries(newlang_checked_c_node PUBLIC newlang_semantic)
add_executable(newlangc src/main.c)
target_link_libraries(newlangc PRIVATE newlang_checked_c_node newlang_semantic newlang_diagnostic)

include(CTest)
if(BUILD_TESTING)
    find_package(Python3 3.12 REQUIRED COMPONENTS Interpreter)
    add_executable(diagnostic_test tests/unit/diagnostic_test.c)
    target_link_libraries(diagnostic_test PRIVATE newlang_diagnostic)
    add_test(NAME diagnostics.unit COMMAND diagnostic_test)

    add_executable(source_test tests/unit/source_test.c)
    target_link_libraries(source_test PRIVATE newlang_source)
    # Linux-only fault injection in tests; no production allocator interface.
    target_link_options(source_test PRIVATE -Wl,--wrap=malloc)
    add_test(NAME source.unit COMMAND source_test)

    add_executable(lexer_test tests/unit/lexer_test.c)
    target_link_libraries(lexer_test PRIVATE newlang_lexer)
    add_test(NAME lexer.unit COMMAND lexer_test)

    add_executable(lexical_frontend_test tests/integration/lexical_frontend_test.c)
    target_link_libraries(lexical_frontend_test PRIVATE newlang_lexer)
    target_link_options(lexical_frontend_test PRIVATE -Wl,--wrap=realloc)
    add_test(NAME source_lexer.integration COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/integration/lexical_frontend_test.py"
        "$<TARGET_FILE:lexical_frontend_test>")

    add_executable(parser_test tests/unit/parser_test.c)
    target_link_libraries(parser_test PRIVATE newlang_parser)
    target_link_options(parser_test PRIVATE -Wl,--wrap=malloc)
    foreach(group type expression loan ownership limits diagnostics)
        add_test(NAME parser_${group}.unit COMMAND parser_test ${group})
    endforeach()
    add_executable(syntax_frontend_test tests/integration/syntax_frontend_test.c)
    target_link_libraries(syntax_frontend_test PRIVATE newlang_parser)
    add_test(NAME source_parser.integration COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/integration/syntax_frontend_test.py"
        "$<TARGET_FILE:syntax_frontend_test>")

    add_executable(semantic_test tests/unit/semantic_test.c)
    target_link_libraries(semantic_test PRIVATE newlang_semantic newlang_parser)
    target_link_options(semantic_test PRIVATE -Wl,--wrap=malloc -Wl,--wrap=realloc)
    foreach(group type value_use domain transition loan failure)
        add_test(NAME semantic_${group}.unit COMMAND semantic_test ${group})
    endforeach()
    add_executable(semantic_frontend_test tests/integration/semantic_frontend_test.c)
    target_link_libraries(semantic_frontend_test PRIVATE newlang_semantic newlang_parser)
    add_test(NAME source_semantic.integration COMMAND semantic_frontend_test)

    add_executable(raw_storage_test tests/unit/raw_storage_test.c)
    target_link_libraries(raw_storage_test PRIVATE newlang_semantic newlang_parser)
    target_link_options(raw_storage_test PRIVATE -Wl,--wrap=malloc -Wl,--wrap=realloc)
    foreach(group claim byte copy access current failure limit diagnostic reborrow partition)
        add_test(NAME raw_${group}.unit COMMAND raw_storage_test ${group})
    endforeach()
    add_executable(raw_storage_cycle_test tests/integration/raw_storage_cycle_test.c)
    target_link_libraries(raw_storage_cycle_test PRIVATE newlang_semantic newlang_parser)
    add_test(NAME raw_storage_cycle.integration COMMAND raw_storage_cycle_test)

    add_executable(source_slice_test tests/unit/source_slice_test.c)
    target_link_libraries(source_slice_test PRIVATE newlang_semantic newlang_parser)
    target_link_options(source_slice_test PRIVATE -Wl,--wrap=malloc -Wl,--wrap=realloc)
    foreach(group parser binding receiving aggregate failure diagnostic)
        add_test(NAME p5_${group}.unit COMMAND source_slice_test ${group})
    endforeach()

    add_executable(source_slice_frontend_test tests/integration/source_slice_frontend_test.c)
    target_link_libraries(source_slice_frontend_test PRIVATE newlang_semantic newlang_parser)
    add_test(NAME p5_source_checked.integration COMMAND source_slice_frontend_test)

    add_executable(sum_slice_test tests/unit/sum_slice_test.c)
    target_link_libraries(sum_slice_test PRIVATE newlang_semantic newlang_parser)
    target_link_options(sum_slice_test PRIVATE -Wl,--wrap=malloc -Wl,--wrap=realloc)
    foreach(group parser registry consuming borrowed storage lifetime failure)
        add_test(NAME p6_${group}.unit COMMAND sum_slice_test ${group})
    endforeach()

    add_executable(sum_frontend_test tests/integration/sum_frontend_test.c)
    target_link_libraries(sum_frontend_test PRIVATE newlang_semantic newlang_parser)
    add_test(NAME p6_source_checked.integration COMMAND sum_frontend_test)

    add_executable(ref_join_test tests/unit/ref_join_test.c)
    target_link_libraries(ref_join_test PRIVATE newlang_semantic newlang_parser)
    target_link_options(ref_join_test PRIVATE -Wl,--wrap=malloc -Wl,--wrap=realloc)
    foreach(group lifecycle boundary invalid identity failure)
        add_test(NAME p7_${group}.unit COMMAND ref_join_test ${group})
    endforeach()

    add_executable(ref_join_frontend_test tests/integration/ref_join_frontend_test.c)
    target_link_libraries(ref_join_frontend_test PRIVATE newlang_semantic newlang_parser)
    add_test(NAME p7_source_checked.integration COMMAND ref_join_frontend_test)

    add_executable(function_body_test tests/unit/function_body_test.c)
    target_link_libraries(function_body_test PRIVATE newlang_semantic newlang_parser)
    target_link_options(function_body_test PRIVATE -Wl,--wrap=malloc -Wl,--wrap=realloc)
    foreach(group registration exit ownership failure)
        add_test(NAME p8_${group}.unit COMMAND function_body_test ${group})
    endforeach()

    add_executable(function_body_frontend_test tests/integration/function_body_frontend_test.c)
    target_link_libraries(function_body_frontend_test PRIVATE newlang_semantic newlang_parser)
    add_test(NAME p8_source_checked.integration COMMAND function_body_frontend_test)

    add_executable(return_test tests/unit/return_test.c)
    target_link_libraries(return_test PRIVATE newlang_semantic newlang_parser)
    target_link_options(return_test PRIVATE -Wl,--wrap=malloc -Wl,--wrap=realloc)
    foreach(group syntax transfer exit failure effects availability)
        add_test(NAME p9_${group}.unit COMMAND return_test ${group})
    endforeach()
    add_test(NAME p9_decoder.integration COMMAND return_test workload)

    add_executable(function_source_test tests/unit/function_source_test.c)
    target_link_libraries(function_source_test PRIVATE newlang_semantic newlang_parser)
    target_link_options(function_source_test PRIVATE -Wl,--wrap=malloc -Wl,--wrap=realloc)
    foreach(group grammar names visibility precision behavior failure limits)
        add_test(NAME p11_${group}.unit COMMAND function_source_test ${group})
    endforeach()

    add_executable(structural_name_test tests/unit/structural_name_test.c)
    target_link_libraries(structural_name_test PRIVATE newlang_semantic newlang_parser)
    target_link_options(structural_name_test PRIVATE -Wl,--wrap=malloc -Wl,--wrap=realloc)
    foreach(group ingress header member failure)
        add_test(NAME p12_${group}.unit COMMAND structural_name_test ${group})
    endforeach()

    add_executable(control_test tests/unit/control_test.c)
    target_link_libraries(control_test PRIVATE newlang_semantic)
    target_link_options(control_test PRIVATE -Wl,--wrap=malloc -Wl,--wrap=realloc)
    foreach(group exits header origins failure integration)
        add_test(NAME p14_pre_${group}.unit COMMAND control_test ${group})
    endforeach()

    add_executable(loop_test tests/unit/loop_test.c)
    target_link_libraries(loop_test PRIVATE newlang_semantic newlang_parser)
    target_link_options(loop_test PRIVATE -Wl,--wrap=malloc -Wl,--wrap=realloc)
    foreach(workload RANGE 1 40)
        add_test(NAME p14_w${workload}.unit COMMAND loop_test ${workload})
    endforeach()

    add_executable(if_test tests/unit/if_test.c)
    target_link_libraries(if_test PRIVATE newlang_semantic newlang_parser)
    target_link_options(if_test PRIVATE -Wl,--wrap=malloc -Wl,--wrap=realloc)
    foreach(group grammar bool flow identity ref effects nested failure limits)
        add_test(NAME p13_${group}.unit COMMAND if_test ${group})
    endforeach()

    add_executable(unit_name_test tests/unit/unit_name_test.c)
    target_link_libraries(unit_name_test PRIVATE newlang_semantic newlang_parser)
    target_link_options(unit_name_test PRIVATE -Wl,--wrap=malloc -Wl,--wrap=realloc)
    foreach(group admission builtin failure)
        add_test(NAME p10_${group}.unit COMMAND unit_name_test ${group})
    endforeach()

    add_executable(argument_compatibility_test tests/unit/argument_compatibility_test.c)
    target_link_libraries(argument_compatibility_test PRIVATE newlang_semantic newlang_parser)
    add_test(NAME r4_01_argument_compatibility.unit COMMAND argument_compatibility_test)

    add_executable(llvm_smoke tests/integration/llvm_smoke.c)
    target_link_libraries(llvm_smoke PRIVATE newlang_options newlang_llvm)
    add_test(NAME llvm.c_api COMMAND llvm_smoke)

    foreach(case version help no_arguments invalid_option extra_argument)
        add_test(NAME cli.${case} COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/integration/cli_test.py"
            "$<TARGET_FILE:newlangc>" "${case}")
    endforeach()
    add_executable(local_root_test tests/unit/local_root_test.c)
    target_link_libraries(local_root_test PRIVATE newlang_semantic newlang_parser)
    target_link_options(local_root_test PRIVATE -Wl,--wrap=malloc -Wl,--wrap=realloc)
    foreach(group parser evidence direct_write_root mutation negatives conflict resource failures)
        add_test(NAME local_root_${group}.unit COMMAND local_root_test ${group})
    endforeach()
    add_test(NAME local_root_checked_c.integration
        COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/integration/checked_c_local_root_test.py"
        "$<TARGET_FILE:newlangc>" "${CMAKE_C_COMPILER}")
    add_executable(avs_test tests/unit/avs_test.c)
    target_link_libraries(avs_test PRIVATE newlang_semantic newlang_parser)
    target_link_options(avs_test PRIVATE -Wl,--wrap=malloc -Wl,--wrap=realloc)
    foreach(group parser evidence rejection ownership failures)
        add_test(NAME avs_${group}.unit COMMAND avs_test ${group})
    endforeach()
    add_test(NAME north_star_avs_checked_c.integration
        COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/integration/checked_c_avs_test.py"
        "$<TARGET_FILE:newlangc>" "${CMAKE_C_COMPILER}")
    add_executable(u8_literal_test tests/unit/u8_literal_test.c)
    add_executable(fixed_field_test tests/unit/fixed_field_test.c)
    add_executable(recursive_type_test tests/unit/recursive_type_test.c)
    add_test(NAME allocated_node_source.integration
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/integration/allocated_node_test.py"
        "$<TARGET_FILE:newlangc>" "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/allocated_node_semantic.nl")
    add_test(NAME heap_link_source.integration
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/integration/heap_link_test.py"
        "$<TARGET_FILE:newlangc>" "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/heap_link_semantic.nl")
    add_test(NAME heap_link_failures.unit COMMAND allocated_node_test failures
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/heap_link_semantic.nl")
    add_executable(heap_link_test tests/unit/heap_link_test.c)
    target_link_libraries(heap_link_test PRIVATE newlang_checked_c_node newlang_parser)
    add_test(NAME heap_link_evidence.unit COMMAND heap_link_test
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/heap_link_semantic.nl")
    add_executable(heap_link_support_test tests/unit/heap_link_support_test.c)
    target_link_libraries(heap_link_support_test PRIVATE newlang_semantic newlang_parser)
    add_test(NAME heap_link_supporting_claims.unit COMMAND heap_link_support_test)
    add_executable(heap_link_checked_probe tests/integration/heap_link_checked_probe.c)
    target_link_libraries(heap_link_checked_probe PRIVATE newlang_semantic newlang_parser)
    add_executable(heap_link_backend_test tests/unit/heap_link_backend_test.c)
    target_link_libraries(heap_link_backend_test PRIVATE newlang_checked_c_node newlang_parser)
    target_link_options(heap_link_backend_test PRIVATE -Wl,--wrap=malloc)
    add_test(NAME heap_link_backend.unit COMMAND heap_link_backend_test
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/heap_link_semantic.nl")
    add_test(NAME heap_link_native.integration
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/integration/heap_link_native_test.py"
        "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:heap_link_checked_probe>" "${CMAKE_C_COMPILER}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/heap_link_semantic.nl" "${NEWLANG_SANITIZER}")
    add_test(NAME allocated_node_native.integration
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/integration/allocated_native_test.py"
        "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:allocated_checked_probe>" "${CMAKE_C_COMPILER}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/allocated_node_semantic.nl" "${NEWLANG_SANITIZER}")
    add_executable(allocated_backend_test tests/unit/allocated_backend_test.c)
    target_link_libraries(allocated_backend_test PRIVATE newlang_checked_c_node newlang_parser)
    target_link_options(allocated_backend_test PRIVATE -Wl,--wrap=malloc)
    add_test(NAME allocated_backend.unit COMMAND allocated_backend_test
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/allocated_node_semantic.nl")
    add_executable(allocated_checked_probe tests/integration/allocated_checked_probe.c)
    target_link_libraries(allocated_checked_probe PRIVATE newlang_semantic newlang_parser)
    add_executable(handoff_backend_test tests/unit/handoff_backend_test.c)
    target_link_libraries(handoff_backend_test PRIVATE newlang_semantic newlang_parser newlang_checked_c_node)
    target_link_options(handoff_backend_test PRIVATE -Wl,--wrap=malloc)
    add_test(NAME live_tail_handoff_backend.unit COMMAND handoff_backend_test "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/live_tail_handoff.nl")
    add_executable(handoff_checked_probe tests/integration/handoff_checked_probe.c)
    target_link_libraries(handoff_checked_probe PRIVATE newlang_semantic newlang_parser)
    add_test(NAME live_tail_handoff_native.integration
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/integration/handoff_native_test.py"
        "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:handoff_checked_probe>" "${CMAKE_C_COMPILER}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/live_tail_handoff.nl" "${NEWLANG_SANITIZER}")
    add_executable(typed_owner_test tests/unit/typed_owner_test.c)
    target_link_libraries(typed_owner_test PRIVATE newlang_semantic newlang_parser newlang_checked_c_node)
    target_link_options(typed_owner_test PRIVATE -Wl,--wrap=malloc -Wl,--wrap=realloc)
    foreach(group definition evidence precision faults)
        add_test(NAME typed_owner_${group}.unit COMMAND typed_owner_test ${group} ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/live_tail_handoff.nl)
    endforeach()
    add_test(NAME typed_owner_source.integration COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/integration/typed_owner_test.py $<TARGET_FILE:newlangc> ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/live_tail_handoff.nl $<TARGET_FILE:typed_owner_test>)

    add_executable(two_heap_checked_probe tests/integration/two_heap_checked_probe.c)
    target_link_libraries(two_heap_checked_probe PRIVATE newlang_semantic newlang_parser)
    add_executable(two_heap_backend_test tests/unit/two_heap_backend_test.c)
    target_link_libraries(two_heap_backend_test PRIVATE newlang_checked_c_node newlang_parser)
    target_link_options(two_heap_backend_test PRIVATE -Wl,--wrap=malloc)
    add_test(NAME two_heap_backend_lifecycle.unit COMMAND two_heap_backend_test
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/two_heap_semantic.nl")
    add_test(NAME two_heap_backend.unit COMMAND heap_link_backend_test
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/two_heap_semantic.nl")
    add_test(NAME two_heap_native.integration
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/integration/two_heap_native_test.py"
        "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:two_heap_checked_probe>" "${CMAKE_C_COMPILER}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/two_heap_semantic.nl" "${NEWLANG_SANITIZER}")
    add_executable(two_heap_test tests/unit/two_heap_test.c)
    target_link_libraries(two_heap_test PRIVATE newlang_checked_c_node newlang_parser)
    add_test(NAME two_heap_fields.unit COMMAND two_heap_test
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/two_heap_semantic.nl")
    add_test(NAME two_heap_lifecycle.unit COMMAND allocated_join_test
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/two_heap_semantic.nl")
    add_test(NAME two_heap_failures.unit COMMAND allocated_node_test failures
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/two_heap_semantic.nl")
    add_test(NAME two_heap_source.integration
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/integration/two_heap_test.py"
        "$<TARGET_FILE:newlangc>" "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/two_heap_semantic.nl" "$<TARGET_FILE:two_heap_test>")
    add_executable(allocated_join_test tests/unit/allocated_join_test.c)
    target_link_libraries(allocated_join_test PRIVATE newlang_checked_c_node newlang_parser)
    add_test(NAME allocated_join_evidence.unit COMMAND allocated_join_test
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/nested_allocation_join.nl")
    add_test(NAME allocated_join_failures.unit COMMAND allocated_node_test failures
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/nested_allocation_join.nl")
    add_test(NAME allocated_join_source.integration
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/integration/allocated_join_test.py"
        "$<TARGET_FILE:newlangc>" "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/nested_allocation_join.nl")
    add_executable(allocated_node_test tests/unit/allocated_node_test.c)
    target_link_libraries(allocated_node_test PRIVATE newlang_semantic newlang_parser)
    target_link_options(allocated_node_test PRIVATE -Wl,--wrap=malloc -Wl,--wrap=realloc)
    foreach(group evidence operands failures)
        add_test(NAME allocated_node_${group}.unit COMMAND allocated_node_test ${group}
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/allocated_node_semantic.nl")
    endforeach()
    add_executable(node_link_test tests/unit/node_link_test.c)
    add_executable(node_checked_probe tests/integration/node_checked_probe.c)
    target_link_libraries(node_checked_probe PRIVATE newlang_semantic)
    add_executable(checked_c_node_test tests/unit/checked_c_node_test.c)
    target_link_libraries(checked_c_node_test PRIVATE newlang_checked_c_node)
    target_link_options(checked_c_node_test PRIVATE -Wl,--wrap=malloc)
    add_test(NAME checked_c_node.unit COMMAND checked_c_node_test
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/node_link_semantic.nl")
    target_link_libraries(node_link_test PRIVATE newlang_semantic newlang_parser)
    target_link_options(node_link_test PRIVATE -Wl,--wrap=malloc -Wl,--wrap=realloc)
    foreach(group evidence transitions dependencies negatives liveness resource failures)
        add_test(NAME node_link_${group}.unit COMMAND node_link_test ${group}
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/node_link_semantic.nl")
    endforeach()
    add_test(NAME node_topology.integration
        COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/integration/node_topology_test.py"
        "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:node_checked_probe>"
        "${CMAKE_C_COMPILER}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/node_link_semantic.nl"
        "${NEWLANG_SANITIZER}")
    add_test(NAME node_link_source.integration
        COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/integration/node_link_source_test.py"
        "$<TARGET_FILE:newlangc>"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/node_link_semantic.nl")
    target_link_libraries(recursive_type_test PRIVATE newlang_semantic newlang_parser)
    target_link_options(recursive_type_test PRIVATE -Wl,--wrap=malloc -Wl,--wrap=realloc)
    foreach(group parser identity source cycle negatives failures)
        add_test(NAME recursive_type_${group}.unit COMMAND recursive_type_test ${group})
    endforeach()
    add_test(NAME recursive_declaration.integration
        COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/integration/recursive_declaration_test.py"
        "$<TARGET_FILE:newlangc>")
    target_link_libraries(fixed_field_test PRIVATE newlang_semantic newlang_parser)
    target_link_options(fixed_field_test PRIVATE -Wl,--wrap=malloc -Wl,--wrap=realloc)
    foreach(group parser state evidence dependencies negatives failures resource)
        add_test(NAME fixed_field_${group}.unit COMMAND fixed_field_test ${group})
    endforeach()
    add_test(NAME fixed_field_checked_c.integration
        COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/integration/checked_c_fixed_field_test.py"
        "$<TARGET_FILE:newlangc>" "${CMAKE_C_COMPILER}")
    target_link_libraries(u8_literal_test PRIVATE newlang_semantic newlang_parser)
    target_link_options(u8_literal_test PRIVATE -Wl,--wrap=malloc -Wl,--wrap=realloc)
    foreach(group parser values body failures)
        add_test(NAME u8_${group}.unit COMMAND u8_literal_test ${group})
    endforeach()
    add_test(NAME north_star_v1_checked_c.integration
        COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/integration/checked_c_v1_test.py"
        "$<TARGET_FILE:newlangc>" "${CMAKE_C_COMPILER}")
    add_test(NAME north_star_v0_checked_c.integration
        COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/integration/checked_c_v0_test.py"
        "$<TARGET_FILE:newlangc>" "${CMAKE_C_COMPILER}")
    add_test(NAME artifacts.integrity COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/integration/artifact_test.py")
    add_test(NAME oracle.adapter COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/oracle/adapter_test.py")
    add_test(NAME oracle.smoke COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/oracle/smoke.py"
        "$<TARGET_FILE:newlangc>")
    get_property(p0_tests DIRECTORY PROPERTY TESTS)
    set_tests_properties(${p0_tests} PROPERTIES TIMEOUT 90
        ENVIRONMENT "PYTHONDONTWRITEBYTECODE=1;PYTHONHASHSEED=0;LC_ALL=C;ASAN_OPTIONS=detect_leaks=1:halt_on_error=1;UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1")
endif()
