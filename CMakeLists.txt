cmake_minimum_required(VERSION 4.0)

project(
  Falcon
  VERSION 0.1.0
  DESCRIPTION "A fast & lightweight CPP http server lib"
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_library(
  LLHTTP_LIB
  NAMES llhttp
  PATHS "${CMAKE_SOURCE_DIR}/external/llhttp"
  NO_DEFAULT_PATH
)

find_library(
  R3_LIB
  NAMES r3
  PATHS "${CMAKE_SOURCE_DIR}/external/r3/.libs"
  NO_DEFAULT_PATH
)

file(GLOB_RECURSE FALCON_SOURCES "${CMAKE_SOURCE_DIR}/src/*.cpp")
add_library(falcon STATIC ${FALCON_SOURCES})
target_include_directories(falcon PUBLIC ${CMAKE_SOURCE_DIR}/)

add_executable(example example.cpp)
target_link_libraries(example PRIVATE falcon uv ${LLHTTP_LIB} ${R3_LIB})

# enable_testing()
# add_subdirectory(tests)
