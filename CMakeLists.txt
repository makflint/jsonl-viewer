cmake_minimum_required(VERSION 3.14)
project(session_pretty_print LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(catch2 third_party/catch_amalgamated.cpp)
target_include_directories(catch2 PUBLIC third_party)

add_executable(tests tests/parser_test.cpp)
target_link_libraries(tests catch2)
target_include_directories(tests PRIVATE src)
