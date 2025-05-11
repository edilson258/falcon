include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

file(GLOB tests_sources "test_*.cpp")
foreach(test_source ${tests_sources})
  get_filename_component(test_name ${test_source} NAME_WE)
  add_executable(${test_name} ${test_source})
  target_include_directories(${test_name} PRIVATE ${CMAKE_SOURCE_DIR}/)
  target_link_libraries(
    ${test_name}
    falcon
    GTest::gtest_main
  )
  gtest_discover_tests(${test_name})
  add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()
