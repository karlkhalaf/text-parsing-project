cmake_minimum_required(VERSION 3.16)
project(text_parsing_project LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(regex_core
    src/dfa.cpp
    src/nfa.cpp
    src/regex_parser.cpp
)
target_include_directories(regex_core PUBLIC src)

add_executable(regex_matcher src/main.cpp)
target_link_libraries(regex_matcher PRIVATE regex_core)

enable_testing()

add_executable(test_dfa tests/test_dfa.cpp)
target_link_libraries(test_dfa PRIVATE regex_core)
add_test(NAME test_dfa COMMAND test_dfa)

add_executable(test_regex tests/test_regex.cpp)
target_link_libraries(test_regex PRIVATE regex_core)
add_test(NAME test_regex COMMAND test_regex)
