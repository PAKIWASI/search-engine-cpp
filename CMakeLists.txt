cmake_minimum_required(VERSION 3.20)
project(search_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB SRC_FILES "${CMAKE_SOURCE_DIR}/src/*.cpp")
file(GLOB HEADER_FILES "${CMAKE_SOURCE_DIR}/include/*.hpp")

add_executable(main ${SRC_FILES} ${HEADER_FILES})

target_include_directories(main PRIVATE ${CMAKE_SOURCE_DIR}/include)



if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(STATUS "Using Clang compiler")
    target_compile_options(main PRIVATE 
        -Wall 
        -Wextra 
        -Wpedantic
    )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    message(STATUS "Using GCC compiler")
    target_compile_options(main PRIVATE 
        -Wall 
        -Wextra 
        -Wpedantic
    )
endif()


