# Path: c:\Users\aruna\Desktop\backtest\CMakeLists.txt
cmake_minimum_required(VERSION 3.16)
project(nemo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Gather all source files recursively
file(GLOB_RECURSE NEMO_SOURCES
    ${CMAKE_SOURCE_DIR}/src/*.cpp
    ${CMAKE_SOURCE_DIR}/src/*/*.cpp
    ${CMAKE_SOURCE_DIR}/src/*/*/*.cpp
)

add_executable(nemo ${NEMO_SOURCES})

# Optionally, copy data and config folders to build dir for convenience
add_custom_command(TARGET nemo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/data $<TARGET_FILE_DIR:nemo>/data)
add_custom_command(TARGET nemo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/logs $<TARGET_FILE_DIR:nemo>/logs)
