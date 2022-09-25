cmake_minimum_required(VERSION 3.5)



set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()

include_directories (${SEARCHSERVER_SOURCE_DIR}/src) 

set(TEST_HEADERS logduration.h test_runner.h)

set(TEST_SRCS)

add_executable(
    ${PROJECT_NAME}_Test
    ${TEST_HEADERS} 
    ${TEST_SRCS}
    search_server_test.cpp
    )

target_include_directories(${PROJECT_NAME}_Test PRIVATE SearchServer)
target_link_libraries(${PROJECT_NAME}_Test SearchServer)

add_executable(
    ${PROJECT_NAME}_Benchmark
    ${TEST_HEADERS} 
    ${TEST_SRCS}
    search_server_benchmark.cpp
    )

target_include_directories(${PROJECT_NAME}_Benchmark PRIVATE SearchServer)
target_link_libraries(${PROJECT_NAME}_Benchmark SearchServer)
