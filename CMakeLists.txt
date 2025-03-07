cmake_minimum_required(VERSION 3.21)
project(lzxd_cpp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_library(${PROJECT_NAME} STATIC ${SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC "include")

# Testing
file(GLOB_RECURSE TEST_SOURCES "test/test.cpp")

if (TEST_SOURCES)
    add_executable(${PROJECT_NAME}_test ${TEST_SOURCES})
    target_link_libraries(${PROJECT_NAME}_test PRIVATE ${PROJECT_NAME})
endif()
