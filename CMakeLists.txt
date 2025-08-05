cmake_minimum_required(VERSION 3.29)

project(
        Falcon
        VERSION 0.1.0
        DESCRIPTION "A fast & lightweight CPP http server library"
        LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_subdirectory(deps/r3)
add_subdirectory(deps/libuv)
add_subdirectory(deps/llhttp)
add_subdirectory(deps/spdlog)

add_library(falcon SHARED
        src/falcon.cc
        src/http.cc
        src/request.cc
        src/response.cc
        src/router.cc
        src/utils.cc)

target_link_libraries(falcon PUBLIC r3 uv llhttp_shared spdlog::spdlog)

target_include_directories(falcon PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_include_directories(falcon
        SYSTEM
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/deps/r3/include
        ${CMAKE_CURRENT_SOURCE_DIR}/deps/libuv/include
        ${CMAKE_CURRENT_SOURCE_DIR}/deps/llhttp/include
        ${CMAKE_CURRENT_SOURCE_DIR}/deps/spdlog/include
        ${CMAKE_CURRENT_SOURCE_DIR}/deps/nljson/single_include/nlohmann)

#set_target_properties(falcon PROPERTIES POSITION_INDEPENDENT_CODE ON)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(falcon PUBLIC
            -Wall
            -Wextra
            -Wpedantic
            -Wunreachable-code
            -Wunused
            -Wpointer-arith
            -Wcast-align
            -Wwrite-strings
            -Wcast-qual
            -Wswitch-default
            -Wswitch-enum
            -Wconversion
            -Wdeprecated
            -Wformat

            $<$<CONFIG:Debug>:-O0 -g -fsanitize=address -fsanitize=undefined>
            $<$<CONFIG:Release>:-O3 -DNDEBUG -flto>
    )
    target_link_options(falcon PUBLIC
            $<$<CONFIG:Debug>:-fsanitize=address -fsanitize=undefined>
            $<$<CONFIG:Release>:-flto>
    )

elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(falcon PRIVATE
            # MSVC warning flags
            /W4
            /WX-
            /permissive-
            $<$<CONFIG:Debug>:/Od /Zi /RTC1>
            $<$<CONFIG:Release>:/O2 /DNDEBUG /GL>)

else ()
    message(WARNING "Unknown compiler: ${CMAKE_CXX_COMPILER_ID}")
endif ()

add_subdirectory(examples)

enable_testing()
add_subdirectory(tests)
