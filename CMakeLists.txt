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
add_executable(newlangc src/main.c)
target_link_libraries(newlangc PRIVATE newlang_diagnostic)

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

    add_executable(llvm_smoke tests/integration/llvm_smoke.c)
    target_link_libraries(llvm_smoke PRIVATE newlang_options newlang_llvm)
    add_test(NAME llvm.c_api COMMAND llvm_smoke)

    foreach(case version help no_arguments invalid_option extra_argument unsupported)
        add_test(NAME cli.${case} COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/integration/cli_test.py"
            "$<TARGET_FILE:newlangc>" "${case}")
    endforeach()
    add_test(NAME artifacts.integrity COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/integration/artifact_test.py")
    add_test(NAME oracle.smoke COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/oracle/smoke.py"
        "$<TARGET_FILE:newlangc>")
    get_property(p0_tests DIRECTORY PROPERTY TESTS)
    set_tests_properties(${p0_tests} PROPERTIES TIMEOUT 90
        ENVIRONMENT "PYTHONDONTWRITEBYTECODE=1;PYTHONHASHSEED=0;LC_ALL=C;ASAN_OPTIONS=detect_leaks=1:halt_on_error=1;UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1")
endif()
