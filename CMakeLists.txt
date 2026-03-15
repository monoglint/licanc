cmake_minimum_required(VERSION 4.2.3 FATAL_ERROR)

# set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD d0edc3af-4c50-42ea-a356-e2862fe7a444)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_FLAGS "-stdlib=libc++")

set(CMAKE_CXX_MODULE_STD OFF)

project(licanc LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(licanc
    src/main.cc
)

add_subdirectory(src)
target_link_libraries(licanc PUBLIC
    driver
    frontend
    manager
    manager_types
    util
)