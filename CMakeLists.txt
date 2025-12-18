cmake_minimum_required(VERSION 3.20)
project(search_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB SRC_FILES "${CMAKE_SOURCE_DIR}/src/*.cpp")
file(GLOB HEADER_FILES "${CMAKE_SOURCE_DIR}/include/*.hpp")

add_executable(main ${SRC_FILES} ${HEADER_FILES})

target_include_directories(main PRIVATE ${CMAKE_SOURCE_DIR}/include)

# configuring libstemmer lib
find_library(STEMMER_LIB stemmer)
if(STEMMER_LIB)
    message(STATUS "Found libstemmer: ${STEMMER_LIB}")
else()
    message(FATAL_ERROR "libstemmer not found")
endif()

target_link_libraries(main ${STEMMER_LIB})


