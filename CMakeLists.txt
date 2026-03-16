cmake_minimum_required(VERSION 4.2.3 FATAL_ERROR)

set(CMAKE_CXX_COMPILER "C:/msys64/clang64/bin/clang++.exe")
set(CMAKE_CXX_FLAGS "-stdlib=libc++")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES ON) 

project(lican_collection LANGUAGES CXX)

add_subdirectory(licanc) # licanc executable