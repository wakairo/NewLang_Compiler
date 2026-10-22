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

option(NEWLANG_FRONTEND_ONLY "Build/check without LLVM (not the full validation gate)" OFF)
if(NEWLANG_FRONTEND_ONLY)
    message(STATUS "Frontend-only: LLVM discovery and llvm.c_api smoke omitted")
else()
    find_program(LLVM_CONFIG NAMES llvm-config-23 REQUIRED)
    execute_process(COMMAND "${LLVM_CONFIG}" --version
        OUTPUT_VARIABLE llvm_version OUTPUT_STRIP_TRAILING_WHITESPACE
        COMMAND_ERROR_IS_FATAL ANY)
    if(NOT llvm_version STREQUAL "23.1.3")
        message(FATAL_ERROR "LLVM must be exactly 23.1.3; found ${llvm_version}")
    endif()
    # A release-locked executable from bootstrap, not a system LLVM fallback.
    if(NOT DEFINED ENV{NEWLANG_SDK_ROOT} OR "$ENV{NEWLANG_SDK_ROOT}" STREQUAL "")
        message(FATAL_ERROR "Full LLVM CI needs curated NEWLANG_SDK_ROOT from .deps/activate.sh")
    endif()
    if(NOT LLVM_CONFIG STREQUAL "$ENV{NEWLANG_SDK_ROOT}/bin/llvm-config-23")
        message(FATAL_ERROR "LLVM config must come from exact curated SDK prefix")
    endif()
    execute_process(COMMAND "${LLVM_CONFIG}" --includedir
        OUTPUT_VARIABLE llvm_include OUTPUT_STRIP_TRAILING_WHITESPACE
        COMMAND_ERROR_IS_FATAL ANY)
    execute_process(COMMAND "${LLVM_CONFIG}" --libdir
        OUTPUT_VARIABLE llvm_libdir OUTPUT_STRIP_TRAILING_WHITESPACE
        COMMAND_ERROR_IS_FATAL ANY)
    execute_process(COMMAND "${LLVM_CONFIG}" --link-static
        --shared-mode core analysis
        OUTPUT_VARIABLE llvm_shared_mode OUTPUT_STRIP_TRAILING_WHITESPACE
        COMMAND_ERROR_IS_FATAL ANY)
    if(NOT llvm_shared_mode STREQUAL "static")
        message(FATAL_ERROR "Curated LLVM 23.1.3 must be static-only")
    endif()
    if(NOT EXISTS "${llvm_include}/llvm-c/Core.h"
            OR NOT EXISTS "${llvm_include}/llvm-c/Analysis.h"
            OR NOT EXISTS "${llvm_libdir}/libLLVMCore.a")
        message(FATAL_ERROR "Curated LLVM C API static headers/archives missing")
    endif()
    execute_process(COMMAND "${LLVM_CONFIG}" --link-static
        --libs core analysis --system-libs
        OUTPUT_VARIABLE llvm_static_flags OUTPUT_STRIP_TRAILING_WHITESPACE
        COMMAND_ERROR_IS_FATAL ANY)
    separate_arguments(llvm_static_libraries NATIVE_COMMAND
        "${llvm_static_flags}")
    if(NOT llvm_static_libraries)
        message(FATAL_ERROR "Curated LLVM static link closure is empty")
    endif()
    # Only the LLVM C API infrastructure smoke needs the host C++ linker.
    # Production source/checker/backend still compile as strict C17.
    enable_language(CXX)
    add_library(newlang_llvm INTERFACE)
    target_include_directories(newlang_llvm INTERFACE "${llvm_include}")
    target_link_directories(newlang_llvm INTERFACE "${llvm_libdir}")
    target_link_libraries(newlang_llvm INTERFACE ${llvm_static_libraries})
    message(STATUS "LLVM C API ${llvm_version}: curated static LLVM via ${LLVM_CONFIG}")
endif()

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
add_library(newlang_semantic STATIC src/copy_body.c src/if_field.c src/array4.c src/usize_addition.c src/semantic.c src/semantic_check.c src/recursive_type.c src/allocated_node.c src/allocated_join.c src/captured_closure.c src/node_join.c src/node_links.c src/packet_fork.c src/fixed_field.c src/sum.c src/raw_storage.c src/function_body.c src/typed_owner.c src/custody.c src/control.c)
target_link_libraries(newlang_semantic PUBLIC newlang_checked newlang_diagnostic newlang_parser)
add_library(newlang_checked_c_node STATIC src/checked_c_node.c src/checked_c_five.c)
target_link_libraries(newlang_checked_c_node PUBLIC newlang_semantic)
add_executable(newlangc src/main.c)
target_link_libraries(newlangc PRIVATE newlang_checked_c_node newlang_semantic newlang_diagnostic)

include(CTest)
if(BUILD_TESTING)
    find_package(Python3 3.12 REQUIRED COMPONENTS Interpreter)
    # R725: independent, R-owned proof and strict native tests. Do not merge.
    add_executable(r725_public_gate tests/r725/public_gate.c)
    target_link_libraries(r725_public_gate PRIVATE newlang_semantic)
    add_test(NAME r725_independent_blind.integration COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/r725/check.py" "$<TARGET_FILE:newlangc>"
        "$<TARGET_FILE:r725_public_gate>" "${CMAKE_C_COMPILER}" "${NEWLANG_SANITIZER}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/r725")
    # SPECULATIVE PR533 opt-in: default compiler/parser stay OFF. Test-only
    # rebuild of the SAME parser exercises Production semantics/owned/C17.
    add_executable(p718_gate_test tests/p718/gate_test.c)
    target_link_libraries(p718_gate_test PRIVATE newlang_checked_c_node newlang_semantic)
    target_link_options(p718_gate_test PRIVATE -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc)
    add_test(NAME p718_if_return_proof_oom.unit COMMAND p718_gate_test
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p718/return_false.nl" "${CMAKE_CURRENT_SOURCE_DIR}/tests/p718/continue_false.nl")
    add_test(NAME p718_if_return_native.integration COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p718/native.py" "$<TARGET_FILE:newlangc>"
        "$<TARGET_FILE:p718_public_probe>" "${CMAKE_C_COMPILER}" "${NEWLANG_SANITIZER}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p718")
    add_executable(p722_gate_test tests/p722/gate_test.c)
    target_link_libraries(p722_gate_test PRIVATE newlang_checked_c_node newlang_semantic)
    target_link_options(p722_gate_test PRIVATE -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc)
    add_test(NAME p722_callee_return_proof_oom.unit COMMAND p722_gate_test
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p722/before_false.nl" "${CMAKE_CURRENT_SOURCE_DIR}/tests/p722/after_false.nl")
    add_test(NAME p722_callee_return_native.integration COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p722/native.py" "$<TARGET_FILE:newlangc>"
        "$<TARGET_FILE:p722_public_probe>" "${CMAKE_C_COMPILER}" "${NEWLANG_SANITIZER}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p722")
    add_executable(p722_public_probe tests/p722/public_probe.c)
    target_link_libraries(p722_public_probe PRIVATE newlang_parser newlang_semantic)
    add_executable(p718_public_probe tests/p718/public_probe.c)
    target_link_libraries(p718_public_probe PRIVATE newlang_parser newlang_semantic)
    add_executable(p713_gate_test tests/p713/gate_test.c)
    target_link_libraries(p713_gate_test PRIVATE newlang_checked_c_node newlang_semantic)
    target_link_options(p713_gate_test PRIVATE -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc)
    add_test(NAME p713_if_field_proof_oom.unit COMMAND p713_gate_test
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p713/both_true.nl"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p713/one_false.nl"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p713/repeated_true.nl")
    add_test(NAME p713_if_field_native.integration COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p713/native.py" "$<TARGET_FILE:newlangc>"
        "$<TARGET_FILE:p713_public_probe>" "${CMAKE_C_COMPILER}" "${NEWLANG_SANITIZER}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p713")
    add_executable(p713_public_probe tests/p713/public_probe.c)
    target_link_libraries(p713_public_probe PRIVATE newlang_parser newlang_semantic)
    add_executable(p706_gate_test tests/p706/gate_test.c)
    target_link_libraries(p706_gate_test PRIVATE newlang_checked_c_node newlang_semantic)
    target_link_options(p706_gate_test PRIVATE -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc)
    add_test(NAME p706_proof_handoff_oom.unit COMMAND p706_gate_test
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p706/settings.nl" "${CMAKE_CURRENT_SOURCE_DIR}/tests/p706/window.nl")
    add_test(NAME p706_ref_write_native.integration COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p706/native.py" "$<TARGET_FILE:newlangc>"
        "$<TARGET_FILE:p706_public_probe>" "${CMAKE_C_COMPILER}" "${NEWLANG_SANITIZER}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p706")
    add_executable(p706_public_probe tests/p706/public_probe.c)
    target_link_libraries(p706_public_probe PRIVATE newlang_parser newlang_semantic)
    add_executable(p701_gate_test tests/p701/gate_test.c)
    target_link_libraries(p701_gate_test PRIVATE newlang_checked_c_node newlang_semantic)
    target_link_options(p701_gate_test PRIVATE -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc)
    add_test(NAME p701_proof_rollback_oom.unit COMMAND p701_gate_test
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p701/internal_one.nl" "${CMAKE_CURRENT_SOURCE_DIR}/tests/p701/internal_two.nl")
    add_test(NAME p701_stateful_native.integration COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p701/native.py" "$<TARGET_FILE:newlangc>"
        "$<TARGET_FILE:p701_public_probe>" "${CMAKE_C_COMPILER}" "${NEWLANG_SANITIZER}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p701")
    add_executable(p701_public_probe tests/p701/public_probe.c)
    target_link_libraries(p701_public_probe PRIVATE newlang_parser newlang_semantic)
    add_library(newlang_parser_p534 STATIC src/parser.c)
    target_compile_definitions(newlang_parser_p534 PRIVATE NEWLANG_EXPERIMENTAL_OPTION_ARRAY4_SOURCE=1)
    target_link_libraries(newlang_parser_p534 PUBLIC newlang_syntax newlang_lexer newlang_diagnostic)
    add_executable(newlangc_p534 src/main.c)
    target_link_libraries(newlangc_p534 PRIVATE newlang_parser_p534 newlang_checked_c_node newlang_semantic newlang_diagnostic)
    add_executable(p572_public_probe tests/p572/public_probe.c)
    target_link_libraries(p572_public_probe PRIVATE newlang_semantic)
    add_test(NAME p572_three_node_owned.unit COMMAND p572_public_probe "${CMAKE_CURRENT_SOURCE_DIR}/tests/p572/three_originals.nl")
    add_executable(p572_fault_test tests/p572/fault_test.c)
    target_link_libraries(p572_fault_test PRIVATE newlang_checked_c_node newlang_semantic)
    target_link_options(p572_fault_test PRIVATE -Wl,--wrap=malloc -Wl,--wrap=realloc -Wl,--wrap=calloc)
    add_test(NAME p572_three_node_fault.unit COMMAND p572_fault_test "${CMAKE_CURRENT_SOURCE_DIR}/tests/p572/three_originals.nl")
    add_test(NAME p572_source_oom.unit COMMAND p534_semantic_gate oom "${CMAKE_CURRENT_SOURCE_DIR}/tests/p572/three_originals.nl")
    add_test(NAME p572_three_node_native.integration
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/p572/native_test.py"
        "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:p572_public_probe>"
        "$<TARGET_FILE:p534_semantic_gate>" "${CMAKE_C_COMPILER}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p572" "${NEWLANG_SANITIZER}")
    add_executable(p534_public_probe tests/p534/public_probe.c)
    target_link_libraries(p534_public_probe PRIVATE newlang_parser_p534 newlang_semantic)
    add_test(NAME p534_source_owned.unit COMMAND p534_public_probe "${CMAKE_CURRENT_SOURCE_DIR}/tests/p534/original.nl")
    add_executable(p534_semantic_gate tests/p514/gate_test.c)
    target_link_libraries(p534_semantic_gate PRIVATE newlang_parser_p534 newlang_checked_c_node newlang_semantic)
    target_link_options(p534_semantic_gate PRIVATE -Wl,--wrap=malloc -Wl,--wrap=realloc -Wl,--wrap=calloc)
    add_test(NAME p534_oom.unit COMMAND p534_semantic_gate oom "${CMAKE_CURRENT_SOURCE_DIR}/tests/p534/original.nl")
    add_executable(p534_fault_test tests/p534/fault_test.c)
    target_link_libraries(p534_fault_test PRIVATE newlang_parser_p534 newlang_semantic)
    target_link_options(p534_fault_test PRIVATE -Wl,--wrap=malloc -Wl,--wrap=realloc -Wl,--wrap=calloc)
    add_test(NAME p534_issuer_fault.unit COMMAND p534_fault_test "${CMAKE_CURRENT_SOURCE_DIR}/tests/p534/original.nl")
    add_test(NAME p534_array_native.integration
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/p534/native_test.py"
        "$<TARGET_FILE:newlangc_p534>" "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:p534_public_probe>"
        "$<TARGET_FILE:p534_semantic_gate>" "${CMAKE_C_COMPILER}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p534" "${NEWLANG_SANITIZER}")

    add_executable(p541_public_probe tests/p541/public_probe.c)
    target_link_libraries(p541_public_probe PRIVATE newlang_parser_p534 newlang_semantic)
    add_test(NAME p541_source_owned.unit COMMAND p541_public_probe "${CMAKE_CURRENT_SOURCE_DIR}/tests/p541/two_roots.nl")
    add_test(NAME p541_oom.unit COMMAND p534_semantic_gate oom "${CMAKE_CURRENT_SOURCE_DIR}/tests/p541/two_roots.nl")
    add_executable(p541_fault_test tests/p541/fault_test.c)
    target_link_libraries(p541_fault_test PRIVATE newlang_parser_p534 newlang_semantic)
    target_link_options(p541_fault_test PRIVATE -Wl,--wrap=malloc -Wl,--wrap=realloc -Wl,--wrap=calloc)
    add_test(NAME p541_issuer_fault.unit COMMAND p541_fault_test "${CMAKE_CURRENT_SOURCE_DIR}/tests/p541/two_roots.nl")
    add_test(NAME p541_array_native.integration
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/p541/native_test.py"
        "$<TARGET_FILE:newlangc_p534>" "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:p541_public_probe>"
        "$<TARGET_FILE:p534_semantic_gate>" "${CMAKE_C_COMPILER}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p541" "${NEWLANG_SANITIZER}")

    add_executable(p636_index_gate tests/p636/index_gate.c)
    target_link_libraries(p636_index_gate PRIVATE newlang_checked_c_node newlang_semantic)
    add_test(NAME p636_plain_source_indices.unit COMMAND p636_index_gate)
    add_test(NAME p636_local_arm_indices.unit COMMAND p636_index_gate
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p630/then_true.nl")
    add_test(NAME p636_equal_arm_indices.unit COMMAND p636_index_gate
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p631/same_value_local.nl")

    add_test(NAME p634_historical_hash.unit
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/p634/hash_gate_test.py"
        "${CMAKE_CURRENT_SOURCE_DIR}")
    add_executable(p634_ended_root_gate tests/p634/ended_root_gate.c)
    target_link_libraries(p634_ended_root_gate PRIVATE newlang_checked_c_node newlang_semantic)
    add_test(NAME p634_same_value_ended_root.unit COMMAND p634_ended_root_gate
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p631/same_value_local.nl")
    add_test(NAME p634_unequal_ended_root.unit COMMAND p634_ended_root_gate
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p630/both_false.nl")
    add_executable(p633_derivation_gate tests/p633/derivation_gate.c)
    target_link_libraries(p633_derivation_gate PRIVATE newlang_checked_c_node newlang_semantic)
    add_test(NAME p633_equal_outer_derivation.unit COMMAND p633_derivation_gate
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p631/duplicate_outer.nl")
    add_test(NAME p633_local_cli.integration
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/p633/native_test.py"
        "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:p628_public_probe>"
        "$<TARGET_FILE:p633_public_probe>" "$<TARGET_FILE:p517_gate_test>"
        "$<TARGET_FILE:p514_gate_test>" "${CMAKE_C_COMPILER}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p633" "${NEWLANG_SANITIZER}")
    foreach(case both_false outer_true pure_true)
        add_test(NAME p633_${case}_oom.unit COMMAND p514_gate_test oom
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/p630/${case}.nl")
    endforeach()
    add_executable(p633_local_gate tests/p633/local_gate.c)
    target_link_libraries(p633_local_gate PRIVATE newlang_checked_c_node newlang_semantic)
    add_test(NAME p633_local_same_value.unit COMMAND p633_local_gate
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p631/same_value_local.nl")
    add_test(NAME p633_local_different_values.unit COMMAND p633_local_gate
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p630/both_false.nl")
    add_test(NAME p651_cli_native.integration
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/p651/native_test.py"
        "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:p651_gate>" "$<TARGET_FILE:p514_gate_test>"
        "${CMAKE_C_COMPILER}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/p651" "${NEWLANG_SANITIZER}")
    add_executable(p651_gate tests/p651/gate.c)
    target_link_libraries(p651_gate PRIVATE newlang_checked_c_node newlang_semantic)
    target_link_options(p651_gate PRIVATE -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc)
    add_test(NAME p651_true_proof.unit COMMAND p651_gate "${CMAKE_CURRENT_SOURCE_DIR}/tests/p651/one_if_control.nl")
    add_test(NAME p651_false_proof.unit COMMAND p651_gate "${CMAKE_CURRENT_SOURCE_DIR}/tests/p651/false_variant.nl")
    add_test(NAME p651_oom.unit COMMAND p514_gate_test oom "${CMAKE_CURRENT_SOURCE_DIR}/tests/p651/one_if_control.nl")
    add_executable(p633_public_probe tests/p633/public_probe.c)
    target_link_libraries(p633_public_probe PRIVATE newlang_semantic)
    add_executable(p633_origin_gate tests/p633/origin_gate.c)
    target_link_libraries(p633_origin_gate PRIVATE newlang_checked_c_node newlang_semantic)
    add_test(NAME p633_equal_world_origin.unit COMMAND p633_origin_gate
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p631/same_value_local.nl")
    add_test(NAME p633_unequal_world_origin.unit COMMAND p633_origin_gate
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p630/both_false.nl")

    add_executable(p628_public_probe tests/p628/public_probe.c)
    target_link_libraries(p628_public_probe PRIVATE newlang_semantic)

    add_test(NAME p628_cli_pair_if.integration
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/p628/native_test.py"
        "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:p628_public_probe>"
        "$<TARGET_FILE:p517_gate_test>" "$<TARGET_FILE:p626_comparison_gate>"
        "$<TARGET_FILE:p514_gate_test>" "${CMAKE_C_COMPILER}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p628" "${NEWLANG_SANITIZER}")
    foreach(case true false alias)
        add_test(NAME p628_${case}_oom.unit COMMAND p514_gate_test oom
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/p628/pair_${case}.nl")
    endforeach()

    add_executable(p626_public_probe tests/p626/public_probe.c)
    target_link_libraries(p626_public_probe PRIVATE newlang_semantic)
    add_executable(p626_comparison_gate tests/p626/comparison_gate.c)
    target_link_libraries(p626_comparison_gate PRIVATE newlang_checked_c_node newlang_semantic)
    add_test(NAME p626_cli_copy_if.integration
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/p626/native_test.py"
        "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:p626_public_probe>"
        "$<TARGET_FILE:p508_gate_test>" "$<TARGET_FILE:p626_comparison_gate>"
        "${CMAKE_C_COMPILER}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/p626" "${NEWLANG_SANITIZER}")
    add_test(NAME p626_if_oom.unit COMMAND p508_gate_test oom
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p626/fits_true.nl")

    add_executable(p605_public_probe tests/p605/public_probe.c)
    target_link_libraries(p605_public_probe PRIVATE newlang_semantic)
    add_test(NAME p605_arithmetic_comparison.integration
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/p605/integration_test.py"
        "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:p605_public_probe>"
        "${CMAKE_C_COMPILER}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/p605" "${NEWLANG_SANITIZER}")
    add_test(NAME p605_mutant_phase.unit
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/p605/phase_test.py"
        "${CMAKE_C_COMPILER}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/p603/integration_test.py")
    add_executable(p603_public_probe tests/p603/public_probe.c)
    target_link_libraries(p603_public_probe PRIVATE newlang_semantic)
    add_executable(p603_gate_probe tests/p603/gate_probe.c)
    target_link_libraries(p603_gate_probe PRIVATE newlang_checked_c_node newlang_semantic)
    add_executable(p603_comparison_test tests/p603/comparison_test.c)
    target_link_libraries(p603_comparison_test PRIVATE newlang_semantic)
    target_link_options(p603_comparison_test PRIVATE -Wl,--wrap=malloc -Wl,--wrap=realloc)
    foreach(group parser values rollback oom)
        add_test(NAME p603_comparison_${group}.unit COMMAND p603_comparison_test ${group})
    endforeach()
    add_test(NAME p603_comparison_native.integration
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/p603/integration_test.py"
        "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:p603_public_probe>"
        "$<TARGET_FILE:p603_gate_probe>" "${CMAKE_C_COMPILER}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p603" "${NEWLANG_SANITIZER}")
    add_executable(p504_public_probe tests/p504/public_probe.c)
    target_link_libraries(p504_public_probe PRIVATE newlang_semantic)
    add_executable(p504_gate_probe tests/p504/gate_probe.c)
    target_link_libraries(p504_gate_probe PRIVATE newlang_checked_c_node newlang_semantic)
    add_executable(p504_equality_test tests/p504/equality_test.c)
    target_link_libraries(p504_equality_test PRIVATE newlang_semantic)
    target_link_options(p504_equality_test PRIVATE -Wl,--wrap=malloc -Wl,--wrap=realloc)
    foreach(group parser values rollback oom)
        add_test(NAME p504_equality_${group}.unit COMMAND p504_equality_test ${group})
    endforeach()
    add_test(NAME p504_equality_native.integration
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/p504/frontier_test.py"
        "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:p504_public_probe>"
        "$<TARGET_FILE:p504_gate_probe>" "${CMAKE_C_COMPILER}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p504" "${NEWLANG_SANITIZER}")

    add_executable(p508_public_probe tests/p508/public_probe.c)
    target_link_libraries(p508_public_probe PRIVATE newlang_semantic)
    add_executable(p508_gate_test tests/p508/gate_test.c)
    target_link_libraries(p508_gate_test PRIVATE newlang_checked_c_node newlang_semantic)
    target_link_options(p508_gate_test PRIVATE -Wl,--wrap=malloc -Wl,--wrap=realloc -Wl,--wrap=calloc)
    foreach(group poison oom)
        add_test(NAME p508_${group}.unit COMMAND p508_gate_test ${group}
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/p508/select_true.nl")
    endforeach()
    add_test(NAME p508_copy_selection_native.integration
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/p508/native_test.py"
        "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:p508_public_probe>"
        "$<TARGET_FILE:p508_gate_test>" "${CMAKE_C_COMPILER}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p508" "${NEWLANG_SANITIZER}")
    add_executable(p514_public_probe tests/p514/public_probe.c)
    target_link_libraries(p514_public_probe PRIVATE newlang_semantic)
    # R675 independently frozen blind source, public owned, C17/native.
    add_executable(r675_public_probe tests/r675/public_probe.c)
    target_link_libraries(r675_public_probe PRIVATE newlang_semantic newlang_parser)
    add_test(NAME r675_blind_source_owned_native.integration
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/r675/blind_test.py"
        "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:r675_public_probe>"
        "${CMAKE_C_COMPILER}" "${NEWLANG_SANITIZER}")
    # R675 calibrated observability: status visible with normal nonverbose CTest.
    add_test(NAME r675_index1_native_or_backend4.integration
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/r675/blind_test.py"
        "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:r675_public_probe>"
        "${CMAKE_C_COMPILER}" "${NEWLANG_SANITIZER}" "index1")
    set_tests_properties(r675_index1_native_or_backend4.integration PROPERTIES
        SKIP_RETURN_CODE 77)
    foreach(r675_negative
        negative_unknown negative_initializer_type negative_decl_type
        negative_escape negative_replace_type)
        if(r675_negative STREQUAL "negative_decl_type")
            set(r675_negative_test_name "r675_negative_decl_type_source_profile.integration")
        else()
            set(r675_negative_test_name "r675_${r675_negative}_semantic.integration")
        endif()
        add_test(NAME ${r675_negative_test_name}
            COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/r675/blind_test.py"
            "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:r675_public_probe>"
            "${CMAKE_C_COMPILER}" "${NEWLANG_SANITIZER}" "${r675_negative}")
    endforeach()
    # P671 retained bounded nominal field eligibility regression evidence.
    add_executable(p671_public_probe tests/p671/public_probe.c)
    target_link_libraries(p671_public_probe PRIVATE newlang_semantic newlang_parser)
    add_executable(p671_fragment_probe tests/p671/fragment_probe.c)
    target_link_libraries(p671_fragment_probe PRIVATE newlang_semantic newlang_parser)
    add_test(NAME p671_other_fragment.unit COMMAND p671_fragment_probe)
    foreach(case packet_stamp route_stamp pair_control)
        add_test(NAME p671_${case}.integration COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/p671/native.py"
            "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:p671_public_probe>"
            "${CMAKE_C_COMPILER}" "${NEWLANG_SANITIZER}" "${case}.nl")
    endforeach()
    foreach(case packet_stamp route_stamp)
        add_test(NAME p671_${case}_oom.unit COMMAND p514_gate_test oom
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/p670/${case}.nl")
    endforeach()
    add_test(NAME p671_negative.integration COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p671/negative.py"
        "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:p514_gate_test>")

    add_executable(p694_fault_test tests/p694/fault_test.c)
    target_link_libraries(p694_fault_test PRIVATE newlang_checked_c_node newlang_semantic)
    target_link_options(p694_fault_test PRIVATE -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc
        -Wl,--wrap=mkstemp -Wl,--wrap=fdopen -Wl,--wrap=fflush -Wl,--wrap=fclose -Wl,--wrap=rename)
    add_test(NAME p694_named_output.integration COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p694/native.py"
        "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:newlangc_p534>" "$<TARGET_FILE:p694_fault_test>"
        "${CMAKE_C_COMPILER}" "${NEWLANG_SANITIZER}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/p694")

    add_executable(p688_public_probe tests/p688/public_probe.c)
    target_include_directories(p688_public_probe PRIVATE tests/support)
    target_link_libraries(p688_public_probe PRIVATE newlang_semantic newlang_parser)
    add_executable(p688_gate_test tests/p688/gate_test.c)
    target_include_directories(p688_gate_test PRIVATE tests/support)
    target_link_libraries(p688_gate_test PRIVATE newlang_checked_c_node newlang_semantic)
    target_link_options(p688_gate_test PRIVATE -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc)
    add_test(NAME p688_c17_scalar_export.integration COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p688/native.py"
        "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:p688_public_probe>"
        "${CMAKE_C_COMPILER}" "${NEWLANG_SANITIZER}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/p688")
    add_test(NAME p688_selected_call_oom.unit COMMAND p688_gate_test
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p688/threshold.nl" "${CMAKE_CURRENT_SOURCE_DIR}/tests/p688/limit.nl")

    add_executable(p685_public_probe tests/p685/public_probe.c)
    target_include_directories(p685_public_probe PRIVATE tests/support)
    target_link_libraries(p685_public_probe PRIVATE newlang_semantic newlang_parser)
    add_executable(p685_gate_test tests/p685/gate_test.c)
    target_include_directories(p685_gate_test PRIVATE tests/support)
    target_link_libraries(p685_gate_test PRIVATE newlang_checked_c_node newlang_semantic)
    target_link_options(p685_gate_test PRIVATE -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc)
    add_test(NAME p685_zero_formal_u8.integration COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p685/native.py"
        "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:p685_public_probe>"
        "${CMAKE_C_COMPILER}" "${NEWLANG_SANITIZER}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/p685")
    add_test(NAME p685_shared_call_gate_oom.unit COMMAND p685_gate_test
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p685/pump.nl" "${CMAKE_CURRENT_SOURCE_DIR}/tests/p685/sensor.nl")

    # P679: ordinary adopted source, no compiler/issuer changes.
    add_executable(p679_public_probe tests/p679/public_probe.c)
    target_include_directories(p679_public_probe PRIVATE tests/support)
    target_link_libraries(p679_public_probe PRIVATE newlang_semantic newlang_parser)
    add_test(NAME p679_device_settings_swap.integration COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p679/native.py"
        "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:p679_public_probe>"
        "${CMAKE_C_COMPILER}" "${NEWLANG_SANITIZER}"
        "${CMAKE_CURRENT_SOURCE_DIR}/examples/device_settings_swap.nl"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p679/freeze.json"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p679/comparison.c17")
    add_test(NAME p679_invalid_source.integration COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p679/negative.py" "$<TARGET_FILE:newlangc>")

    add_executable(p514_gate_test tests/p514/gate_test.c)
    target_link_libraries(p514_gate_test PRIVATE newlang_checked_c_node newlang_semantic)
    target_link_options(p514_gate_test PRIVATE -Wl,--wrap=malloc -Wl,--wrap=realloc -Wl,--wrap=calloc)
    foreach(group poison oom)
        add_test(NAME p514_${group}.unit COMMAND p514_gate_test ${group}
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/p514/compose_true.nl")
    endforeach()
    add_test(NAME p514_composition_native.integration
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/p514/native_test.py"
        "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:p514_public_probe>"
        "$<TARGET_FILE:p514_gate_test>" "${CMAKE_C_COMPILER}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p514" "${NEWLANG_SANITIZER}")
    add_executable(p517_public_probe tests/p517/public_probe.c)
    target_link_libraries(p517_public_probe PRIVATE newlang_semantic)
    add_executable(p517_gate_test tests/p517/gate_test.c)
    target_link_libraries(p517_gate_test PRIVATE newlang_checked_c_node newlang_semantic)
    target_link_options(p517_gate_test PRIVATE -Wl,--wrap=malloc -Wl,--wrap=realloc -Wl,--wrap=calloc)
    add_test(NAME p517_identity_poison.unit COMMAND p517_gate_test
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p517/r516_true.nl")
    add_test(NAME p517_oom.unit COMMAND p514_gate_test oom
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p517/r516_true.nl")
    add_test(NAME p517_r516_composition_native.integration
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/p517/native_test.py"
        "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:p517_public_probe>"
        "$<TARGET_FILE:p517_gate_test>" "$<TARGET_FILE:p514_gate_test>"
        "${CMAKE_C_COMPILER}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/p517" "${NEWLANG_SANITIZER}")
    add_executable(p521_owned_probe tests/p521/owned_probe.c)
    target_link_libraries(p521_owned_probe PRIVATE newlang_checked_c_node newlang_semantic)
    add_executable(p521_call_gate tests/p521/call_gate.c)
    target_link_libraries(p521_call_gate PRIVATE newlang_checked_c_node newlang_semantic)
    add_test(NAME p521_call_evidence.unit COMMAND p521_call_gate
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p521/combined_true.nl")
    add_test(NAME p521_identity_poison.unit COMMAND p517_gate_test
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p521/combined_true.nl")
    add_test(NAME p521_oom.unit COMMAND p514_gate_test oom
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p521/combined_true.nl")
    add_test(NAME p521_copy_call_composition_native.integration
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/p521/native_test.py"
        "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:p521_owned_probe>"
        "$<TARGET_FILE:p517_public_probe>" "$<TARGET_FILE:p521_call_gate>"
        "$<TARGET_FILE:p514_gate_test>" "${CMAKE_C_COMPILER}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p521" "${NEWLANG_SANITIZER}")
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

    if(NOT NEWLANG_FRONTEND_ONLY)
        add_executable(llvm_smoke tests/integration/llvm_smoke.c)
        target_link_libraries(llvm_smoke PRIVATE newlang_options newlang_llvm)
        set_target_properties(llvm_smoke PROPERTIES LINKER_LANGUAGE CXX)
        add_test(NAME llvm.c_api COMMAND llvm_smoke)
    endif()
    add_test(NAME cli.check_source COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/integration/check_cli_test.py"
        "$<TARGET_FILE:newlangc>" "${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME frontend_build_policy.integration COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/integration/frontend_build_policy_test.py"
        "${CMAKE_COMMAND}" "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_C_COMPILER}")

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
    add_executable(p497_literal_test tests/p497/literal_test.c)
    target_link_libraries(p497_literal_test PRIVATE newlang_semantic newlang_parser)
    target_link_options(p497_literal_test PRIVATE -Wl,--wrap=malloc -Wl,--wrap=realloc)
    foreach(group parser values body failures)
        add_test(NAME p497_usize_${group}.unit COMMAND p497_literal_test ${group})
    endforeach()
    add_executable(p497_public_probe tests/p497/public_probe.c)
    target_link_libraries(p497_public_probe PRIVATE newlang_semantic)
    add_test(NAME p497_usize_native.integration
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/p497/frontier_test.py"
        "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:p497_public_probe>"
        "${CMAKE_C_COMPILER}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/p497")

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
    add_executable(live_return_backend_test tests/unit/live_return_backend_test.c)
    target_link_libraries(live_return_backend_test PRIVATE newlang_semantic newlang_parser newlang_checked_c_node)
    target_link_options(live_return_backend_test PRIVATE -Wl,--wrap=malloc)
    add_test(NAME live_tail_return_backend.unit COMMAND live_return_backend_test "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/live_tail_return.nl")
    add_test(NAME live_tail_return_native.integration
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/integration/live_return_native_test.py"
        "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:handoff_checked_probe>" "${CMAKE_C_COMPILER}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/live_tail_return.nl" "${NEWLANG_SANITIZER}")
    add_executable(live_tail_return_test tests/unit/live_tail_return_test.c)
    target_link_libraries(live_tail_return_test PRIVATE newlang_semantic newlang_parser newlang_checked_c_node)
    target_link_options(live_tail_return_test PRIVATE -Wl,--wrap=malloc -Wl,--wrap=realloc)
    foreach(group evidence oom)
        add_test(NAME live_tail_return_${group}.unit COMMAND live_tail_return_test ${group} ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/live_tail_return.nl)
    endforeach()
    add_test(NAME live_tail_return_source.integration COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/integration/live_tail_return_test.py $<TARGET_FILE:newlangc> ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/live_tail_return.nl $<TARGET_FILE:live_tail_return_test>)
    add_executable(packet_fork_test tests/unit/packet_fork_test.c)
    target_link_libraries(packet_fork_test PRIVATE newlang_semantic newlang_checked_c_node)
    target_link_options(packet_fork_test PRIVATE -Wl,--wrap=malloc -Wl,--wrap=realloc)
    foreach(group evidence oom)
        add_test(NAME packet_fork_${group}.unit COMMAND packet_fork_test ${group} ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/live_tail_packet_fork.nl)
    endforeach()
    add_test(NAME packet_fork_source.integration COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/integration/packet_fork_test.py $<TARGET_FILE:newlangc> ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/live_tail_packet_fork.nl $<TARGET_FILE:packet_fork_test>)
    add_executable(packet_retention_test tests/unit/packet_retention_test.c)
    target_link_libraries(packet_retention_test PRIVATE newlang_semantic newlang_checked_c_node)
    target_link_options(packet_retention_test PRIVATE -Wl,--wrap=malloc -Wl,--wrap=realloc)
    foreach(group evidence oom)
        add_test(NAME packet_retention_${group}.unit COMMAND packet_retention_test ${group} ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/live_tail_packet_retained.nl)
    endforeach()
    add_test(NAME packet_retention_source.integration COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/integration/packet_retention_test.py $<TARGET_FILE:newlangc> ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/live_tail_packet_retained.nl $<TARGET_FILE:packet_retention_test>)
    add_executable(custody_source_test tests/unit/custody_source_test.c)
    target_link_libraries(custody_source_test PRIVATE newlang_semantic newlang_checked_c_node)
    target_link_options(custody_source_test PRIVATE -Wl,--wrap=malloc -Wl,--wrap=realloc)
    foreach(group evidence oom)
        add_test(NAME custody_source_${group}.unit COMMAND custody_source_test ${group} ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/live_tail_custody.nl)
    endforeach()
    add_test(NAME custody_source.integration COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/integration/custody_source_test.py $<TARGET_FILE:newlangc> ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/live_tail_custody.nl $<TARGET_FILE:custody_source_test>)
    add_executable(custody_checked_probe tests/integration/custody_checked_probe.c)
    target_link_libraries(custody_checked_probe PRIVATE newlang_semantic)
    add_test(NAME custody_codegen_oom.unit COMMAND custody_source_test codegen-oom ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/live_tail_custody.nl)
    add_test(NAME custody_native.integration
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/integration/custody_native_test.py
        $<TARGET_FILE:newlangc> $<TARGET_FILE:custody_checked_probe> ${CMAKE_C_COMPILER}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/live_tail_custody.nl ${NEWLANG_SANITIZER})
    add_executable(custody_definition_test tests/unit/custody_definition_test.c)
    target_link_libraries(custody_definition_test PRIVATE newlang_semantic)
    target_link_options(custody_definition_test PRIVATE -Wl,--wrap=malloc -Wl,--wrap=realloc)
    foreach(group evidence oom)
        add_test(NAME custody_definition_${group}.unit COMMAND custody_definition_test ${group} ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/live_tail_custody.nl)
    endforeach()
    add_test(NAME custody_source_hold.integration COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/integration/custody_source_hold_test.py $<TARGET_FILE:newlangc> ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/live_tail_custody.nl $<TARGET_FILE:custody_definition_test>)
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
    add_executable(captured_closure_test tests/unit/captured_closure_test.c)
    target_link_libraries(captured_closure_test PRIVATE newlang_checked_c_node)
    target_link_options(captured_closure_test PRIVATE -Wl,--wrap=malloc -Wl,--wrap=realloc -Wl,--wrap=calloc)
    foreach(sites IN ITEMS 2 3 5)
        if(sites EQUAL 2)
            set(closure_fixture two_allocation_no_links)
        elseif(sites EQUAL 3)
            set(closure_fixture three_allocation_no_links)
        else()
            set(closure_fixture five_allocation_closure_probe)
        endif()
        add_test(NAME captured_closure_${sites}_worlds.unit COMMAND captured_closure_test evidence
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/${closure_fixture}.nl" ${sites})
    endforeach()
    foreach(group IN ITEMS poison oom checker-oom)
        add_test(NAME captured_closure_${group}.unit COMMAND captured_closure_test ${group}
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/five_allocation_closure_probe.nl")
    endforeach()
    add_test(NAME captured_closure_source.integration
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/integration/captured_closure_test.py"
        "$<TARGET_FILE:captured_closure_test>" "$<TARGET_FILE:newlangc>")
    add_test(NAME five_root_evidence.unit COMMAND captured_closure_test full-evidence
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/five_root_three_field.nl" 5)
    foreach(group IN ITEMS poison oom checker-oom)
        add_test(NAME five_root_${group}.unit COMMAND captured_closure_test full-${group}
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/five_root_three_field.nl")
    endforeach()
    add_test(NAME five_root_source.integration
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/integration/five_root_source_test.py"
        "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:captured_closure_test>")
    add_executable(five_checked_probe tests/integration/five_checked_probe.c)
    target_link_libraries(five_checked_probe PRIVATE newlang_semantic newlang_parser)
    add_executable(five_backend_test tests/unit/five_backend_test.c)
    target_link_libraries(five_backend_test PRIVATE newlang_checked_c_node newlang_parser)
    target_link_options(five_backend_test PRIVATE -Wl,--wrap=malloc -Wl,--wrap=realloc -Wl,--wrap=calloc)
    add_test(NAME five_root_backend.unit COMMAND five_backend_test
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/five_root_three_field.nl")
    add_test(NAME five_root_native.integration
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/integration/five_native_test.py"
        "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:five_checked_probe>" "${CMAKE_C_COMPILER}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/five_root_three_field.nl" "${NEWLANG_SANITIZER}")
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
    add_executable(p466_public_probe tests/p466/public_probe.c)
    target_link_libraries(p466_public_probe PRIVATE newlang_semantic)
    add_test(NAME p466_composition.integration COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p466/composition_test.py"
        "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:p466_public_probe>"
        "${CMAKE_C_COMPILER}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/p466" "${NEWLANG_SANITIZER}")
    add_executable(p471_public_probe tests/p471/public_probe.c)
    target_link_libraries(p471_public_probe PRIVATE newlang_semantic)
    add_test(NAME p471_crossroot_frontier.integration COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p471/frontier_test.py"
        "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:p471_public_probe>"
        "$<TARGET_FILE:node_checked_probe>" "$<TARGET_FILE:node_join_native_probe>"
        "${CMAKE_C_COMPILER}" "${CMAKE_CURRENT_SOURCE_DIR}" "${NEWLANG_SANITIZER}")
    set_tests_properties(p471_crossroot_frontier.integration PROPERTIES
        PASS_REGULAR_EXPRESSION "P471 PASS FINITE CROSSROOT: control and both frozen cases observed")
    add_executable(p490_public_probe tests/p490/public_probe.c)
    target_link_libraries(p490_public_probe PRIVATE newlang_semantic)
    add_test(NAME p490_postjoin_frontier.integration COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p490/frontier_test.py"
        "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:p490_public_probe>"
        "$<TARGET_FILE:p471_public_probe>" "$<TARGET_FILE:node_join_native_probe>"
        "${CMAKE_C_COMPILER}" "${CMAKE_CURRENT_SOURCE_DIR}" "${NEWLANG_SANITIZER}")
    set_tests_properties(p490_postjoin_frontier.integration PROPERTIES
        PASS_REGULAR_EXPRESSION "P490 C1 PHYSICAL READ PASS; C2 EXPECTED PRECISION STOP")
    add_executable(node_link_test tests/unit/node_link_test.c)
    add_executable(node_initial_test tests/unit/node_initial_test.c)
    target_link_libraries(node_initial_test PRIVATE newlang_checked_c_node)
    target_link_options(node_initial_test PRIVATE -Wl,--wrap=malloc -Wl,--wrap=realloc -Wl,--wrap=calloc)
    foreach(group controls oom)
        add_test(NAME node_initial_some_${group}.unit COMMAND node_initial_test ${group}
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/node_initial_some.nl")
    endforeach()
    add_test(NAME node_initial_native.integration COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/integration/node_initial_test.py"
        "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:node_initial_test>"
        "${CMAKE_C_COMPILER}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures" "${NEWLANG_SANITIZER}")
    add_executable(node_join_native_test tests/unit/node_join_native_test.c)
    target_link_libraries(node_join_native_test PRIVATE newlang_checked_c_node)
    target_link_options(node_join_native_test PRIVATE -Wl,--wrap=malloc -Wl,--wrap=realloc -Wl,--wrap=calloc)
    add_test(NAME node_join_native_carrier.unit COMMAND node_join_native_test controls
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/node_join_carrier_gate.nl")
    add_test(NAME node_join_native_oom.unit COMMAND node_join_native_test oom
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/node_join_same_locator.nl")
    add_test(NAME node_join_native.integration COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/integration/node_join_native_test.py"
        "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:node_join_native_probe>"
        "${CMAKE_C_COMPILER}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures" "${NEWLANG_SANITIZER}")
    add_executable(node_join_native_probe tests/integration/node_join_native_probe.c)
    target_link_libraries(node_join_native_probe PRIVATE newlang_semantic)
    add_executable(node_checked_probe tests/integration/node_checked_probe.c)
    target_link_libraries(node_checked_probe PRIVATE newlang_semantic)
    add_executable(checked_c_node_test tests/unit/checked_c_node_test.c)
    target_link_libraries(checked_c_node_test PRIVATE newlang_checked_c_node)
    target_link_options(checked_c_node_test PRIVATE -Wl,--wrap=malloc)
    add_test(NAME checked_c_node.unit COMMAND checked_c_node_test
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/node_link_semantic.nl")
    target_link_libraries(node_link_test PRIVATE newlang_semantic newlang_parser)
    target_link_options(node_link_test PRIVATE -Wl,--wrap=malloc -Wl,--wrap=realloc)
    add_executable(node_join_proof_test tests/unit/node_join_proof_test.c)
    target_link_libraries(node_join_proof_test PRIVATE newlang_semantic)
    target_link_options(node_join_proof_test PRIVATE -Wl,--wrap=malloc -Wl,--wrap=realloc -Wl,--wrap=calloc)
    foreach(case two_normal_join join_both_arms join_same_locator join_post_write)
        add_test(NAME node_${case}_public_proof.unit COMMAND node_join_proof_test evidence
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/node_${case}.nl")
    endforeach()
    foreach(group negative oom)
        add_test(NAME node_join_public_${group}.unit COMMAND node_join_proof_test ${group}
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/node_join_same_locator.nl")
    endforeach()
    add_executable(node_join_test tests/unit/node_join_test.c)
    target_link_libraries(node_join_test PRIVATE newlang_checked_c_node newlang_parser)
    target_link_options(node_join_test PRIVATE -Wl,--wrap=malloc -Wl,--wrap=realloc -Wl,--wrap=calloc)
    foreach(case two_normal_join join_actual_none join_other_original join_both_arms join_clear_occurrence join_saved_copy join_same_locator join_post_write)
        add_test(NAME node_${case}.unit COMMAND node_join_test evidence
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/node_${case}.nl")
    endforeach()
    foreach(group negatives owner private-ids dependencies oom)
        add_test(NAME node_join_${group}.unit COMMAND node_join_test ${group}
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/node_two_normal_join.nl")
    endforeach()
    add_test(NAME node_join_both_public_oom.unit COMMAND node_join_proof_test oom
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/node_join_both_arms.nl")
    add_test(NAME node_join_both_issuer_oom.unit COMMAND node_join_test oom
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/node_join_both_arms.nl")
    add_test(NAME node_join_owned_poison.unit COMMAND node_join_test proof-poison
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/node_join_same_locator.nl")
    add_test(NAME node_join_source.integration
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/integration/node_join_source_test.py"
            $<TARGET_FILE:newlangc> $<TARGET_FILE:node_join_test>
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures")
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
    add_executable(nested_u8_checked_probe tests/integration/nested_u8_checked_probe.c)
    target_link_libraries(nested_u8_checked_probe PRIVATE newlang_checked_c_node newlang_semantic)
    add_test(NAME nested_u8_checked_c.integration
        COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/integration/checked_c_nested_u8_test.py"
        "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:nested_u8_checked_probe>" "${CMAKE_C_COMPILER}"
        "${CMAKE_CURRENT_SOURCE_DIR}/evidence/p419")
    add_executable(u8_call_checked_probe tests/integration/u8_call_checked_probe.c)
    target_link_libraries(u8_call_checked_probe PRIVATE newlang_checked_c_node newlang_semantic)
    add_test(NAME u8_call_checked_c.integration
        COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/integration/checked_c_u8_call_test.py"
        "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:u8_call_checked_probe>" "${CMAKE_C_COMPILER}")
    add_test(NAME north_star_v1_checked_c.integration
        COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/integration/checked_c_v1_test.py"
        "$<TARGET_FILE:newlangc>" "${CMAKE_C_COMPILER}")
    add_test(NAME north_star_v0_checked_c.integration
        COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/integration/checked_c_v0_test.py"
        "$<TARGET_FILE:newlangc>" "${CMAKE_C_COMPILER}")
    add_executable(p589_public_probe tests/p589/public_probe.c)
    target_link_libraries(p589_public_probe PRIVATE newlang_semantic)
    add_executable(p589_gate_test tests/p589/gate_test.c)
    target_link_libraries(p589_gate_test PRIVATE newlang_checked_c_node newlang_semantic)
    add_executable(p589_parser_oom_test tests/p589/parser_oom_test.c)
    target_link_libraries(p589_parser_oom_test PRIVATE newlang_semantic)
    target_link_options(p589_parser_oom_test PRIVATE -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc)
    add_test(NAME p589_parser_oom.unit
        COMMAND p589_parser_oom_test "${CMAKE_CURRENT_SOURCE_DIR}/tests/p589/names.nl")
    add_test(NAME p589_usize_addition.integration
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/p589/integration_test.py"
        "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:p589_public_probe>" "$<TARGET_FILE:p589_gate_test>"
        "$<TARGET_FILE:p534_semantic_gate>" "${CMAKE_C_COMPILER}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/p589" "${NEWLANG_SANITIZER}")
    add_executable(p596_public_probe tests/p596/public_probe.c)
    target_link_libraries(p596_public_probe PRIVATE newlang_semantic)
    add_executable(p596_gate_test tests/p596/gate_test.c)
    target_link_libraries(p596_gate_test PRIVATE newlang_checked_c_node newlang_semantic)
    add_executable(p596_parser_oom_test tests/p596/parser_oom_test.c)
    target_link_libraries(p596_parser_oom_test PRIVATE newlang_semantic)
    target_link_options(p596_parser_oom_test PRIVATE -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc)
    add_executable(p596_proof_core_test tests/p596/proof_core_test.c)
    target_link_libraries(p596_proof_core_test PRIVATE newlang_semantic)
    add_test(NAME p596_shared_proof.unit
        COMMAND p596_proof_core_test "${CMAKE_CURRENT_SOURCE_DIR}/tests/p596/literals.nl"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p596/plus.nl"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p596/zero.nl"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/p596/shared_sequence.nl")
    add_test(NAME p596_parser_oom.unit
        COMMAND p596_parser_oom_test "${CMAKE_CURRENT_SOURCE_DIR}/tests/p596/names.nl")
    add_test(NAME p596_usize_subtraction.integration
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/p596/integration_test.py"
        "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:p596_public_probe>" "$<TARGET_FILE:p596_gate_test>"
        "$<TARGET_FILE:p534_semantic_gate>" "${CMAKE_C_COMPILER}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/p596" "${NEWLANG_SANITIZER}")
    add_test(NAME c17_target_model.integration
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/p584/target_model_test.py"
        "$<TARGET_FILE:newlangc>" "$<TARGET_FILE:newlangc_p534>" "${CMAKE_C_COMPILER}"
        "${CMAKE_CURRENT_SOURCE_DIR}" "${NEWLANG_SANITIZER}"
        "$<TARGET_FILE:u8_call_checked_probe>" "$<TARGET_FILE:p521_owned_probe>"
        "$<TARGET_FILE:p534_public_probe>" "$<TARGET_FILE:p541_public_probe>"
        "$<TARGET_FILE:node_join_proof_test>" "$<TARGET_FILE:five_checked_probe>")
    add_test(NAME bootstrap_retry.offline
        COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/integration/bootstrap_retry_test.py")
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
