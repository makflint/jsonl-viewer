cmake_minimum_required(VERSION 3.14)
project(session_pretty_print LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(EMSCRIPTEN)
    add_executable(parser src/wasm_bindings.cpp)
    target_compile_options(parser PRIVATE -O2 -fexceptions)
    target_link_options(parser PRIVATE
        --bind
        -sMODULARIZE=1
        -sEXPORT_NAME=ParserModule
        -sEXPORT_ES6=0
        -fexceptions
    )
    set_target_properties(parser PROPERTIES
        SUFFIX ".js"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/web"
    )
else()
    add_library(catch2 third_party/catch_amalgamated.cpp)
    target_include_directories(catch2 PUBLIC third_party)

    add_executable(tests tests/parser_test.cpp)
    target_link_libraries(tests catch2)
    target_include_directories(tests PRIVATE src)
endif()
