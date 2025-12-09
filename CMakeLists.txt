cmake_minimum_required(VERSION 3.10)
project(licanc VERSION 0.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SOURCES "")

file(GLOB_RECURSE SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cc")

add_executable(licanc
    ${SOURCES}

    resources/resources.rc
)

target_include_directories(licanc PRIVATE ${CMAKE_SOURCE_DIR}/include)

set_property(TARGET licanc PROPERTY CXX_STANDARD 17)