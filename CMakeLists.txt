cmake_minimum_required(VERSION 3.28)

project(
  Falcon
  VERSION 0.1.0
  DESCRIPTION "A fast & lightweight CPP http server"
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_library(
  llhttp_parser
  NAMES llhttp
  PATHS "${CMAKE_SOURCE_DIR}/external/llhttp")

file(GLOB_RECURSE FALCON_SOURCES "${CMAKE_SOURCE_DIR}/src/*.cpp")
add_library(falcon STATIC ${FALCON_SOURCES})
target_include_directories(falcon PRIVATE ${CMAKE_SOURCE_DIR}/)

add_executable(example example.cpp)
target_link_libraries(example PRIVATE falcon uv ${llhttp_parser})
target_include_directories(example PRIVATE ${CMAKE_SOURCE_DIR}/)
