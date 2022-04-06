cmake_minimum_required(VERSION 3.5)

project(cpp-search-server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


set(CPP_SEARCH_SERVER_SRCS
    main.cpp
    document.cpp
    read_input_functions.cpp
    remove_duplicates.cpp
    request_queue.cpp
    search_server.cpp
    string_processing.cpp
    test_example_functions.cpp
    processqueries.cpp
)

add_executable(cpp-search-server ${CPP_SEARCH_SERVER_SRCS})

