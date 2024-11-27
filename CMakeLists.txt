cmake_minimum_required(VERSION 3.28)
project(rtracer)

set(CMAKE_CXX_STANDARD 23)

include_directories(geometry)
include_directories(tools)

find_package(PNG REQUIRED)
include_directories(${PNG_INCLUDE_DIRS})
link_directories(${PNG_LIBRARY_DIRS})

add_executable(rtracer rtracer.cpp)
target_link_libraries(rtracer PRIVATE PNG::PNG)
target_compile_options(rtracer PRIVATE -O3)