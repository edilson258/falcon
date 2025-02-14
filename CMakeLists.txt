cmake_minimum_required(VERSION 3.28)

project(
  Falcon
  VERSION 0.1.0
  DESCRIPTION "A fast & lightweight C http server"
  LANGUAGES C)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_library(
  llhttp_parser
  NAMES llhttp
  PATHS "${CMAKE_SOURCE_DIR}/external/llhttp")

file(GLOB_RECURSE falcon_src "${CMAKE_SOURCE_DIR}/src/*.c")
add_library(falcon STATIC ${falcon_src})
target_include_directories(falcon PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_include_directories(falcon PUBLIC ${CMAKE_SOURCE_DIR}/external/llhttp)
target_include_directories(falcon PUBLIC ${CMAKE_SOURCE_DIR}/external/jack)

add_executable(example example.c)
target_link_libraries(example PRIVATE falcon uv ${llhttp_parser})
target_include_directories(example PUBLIC ${CMAKE_SOURCE_DIR}/include)

add_subdirectory(tests)
