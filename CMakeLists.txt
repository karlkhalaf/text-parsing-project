cmake_minimum_required(VERSION 3.16)
project(text_parsing_project LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(regex_core
    src/dense_dfa.cpp
    src/sfa.cpp
    src/dfa.cpp
    src/nfa.cpp
    src/nfa_builder.cpp
    src/dfa_builder.cpp
    src/matcher.cpp
    src/parallel_matcher.cpp
    src/parem_route.cpp
    src/regex_parser.cpp
)
target_include_directories(regex_core PUBLIC src)

add_executable(regex_matcher src/main.cpp)
target_link_libraries(regex_matcher PRIVATE regex_core)

enable_testing()

add_executable(test_dfa tests/test_dfa.cpp)
target_link_libraries(test_dfa PRIVATE regex_core)
add_test(NAME test_dfa COMMAND test_dfa)

add_executable(test_dense_dfa tests/test_dense_dfa.cpp)
target_link_libraries(test_dense_dfa PRIVATE regex_core)
add_test(NAME test_dense_dfa COMMAND test_dense_dfa)

add_executable(test_regex tests/test_regex.cpp)
target_link_libraries(test_regex PRIVATE regex_core)
add_test(NAME test_regex COMMAND test_regex)

add_executable(test_nfa tests/test_nfa.cpp)
target_link_libraries(test_nfa PRIVATE regex_core)
add_test(NAME test_nfa COMMAND test_nfa)

add_executable(test_nfa_builder tests/test_nfa_builder.cpp)
target_link_libraries(test_nfa_builder PRIVATE regex_core)
add_test(NAME test_nfa_builder COMMAND test_nfa_builder)

add_executable(test_dfa_builder tests/test_dfa_builder.cpp)
target_link_libraries(test_dfa_builder PRIVATE regex_core)
add_test(NAME test_dfa_builder COMMAND test_dfa_builder)

add_executable(test_sequential tests/test_sequential.cpp)
target_link_libraries(test_sequential PRIVATE regex_core)
add_test(NAME test_sequential COMMAND test_sequential)

add_executable(test_parallel tests/test_parallel.cpp)
target_link_libraries(test_parallel PRIVATE regex_core)
add_test(NAME test_parallel COMMAND test_parallel)

add_executable(test_sfa tests/test_sfa.cpp)
target_link_libraries(test_sfa PRIVATE regex_core)
add_test(NAME test_sfa COMMAND test_sfa)

add_executable(test_parem_route tests/test_parem_route.cpp)
target_link_libraries(test_parem_route PRIVATE regex_core)
add_test(NAME test_parem_route COMMAND test_parem_route)
