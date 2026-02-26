cmake_minimum_required(VERSION 3.12)

set(SOURCES "")
file(GLOB_RECURSE SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc")

add_library(licanc
    "${SOURCES}"
)

target_include_directories(licanc
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include"
    INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/.."
)