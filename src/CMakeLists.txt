cmake_minimum_required(VERSION 3.5)

project(SearchServer 
    VERSION 1.0
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


set(SEARCH_SERVER_HEADERS document.h paginator.h 
    read_input_functions.h search_server.h request_queue.h 
    string_processing.h remove_duplicates.h concurrent_map.h)


set(SEARCH_SERVER_SRCS    
    document.cpp
    read_input_functions.cpp
    remove_duplicates.cpp
    request_queue.cpp
    search_server.cpp
    string_processing.cpp    
    processqueries.cpp    
)


add_library(
    ${PROJECT_NAME} 
    ${SEARCH_SERVER_SRCS} 
    ${SEARCH_SERVER_HEADERS})

target_include_directories(${PROJECT_NAME}
    PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    PRIVATE
)
