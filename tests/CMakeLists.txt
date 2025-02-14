enable_testing()

file(GLOB TestSources "*.c")
foreach(TestSource ${TestSources})
  # extract filename without extension and use it as test executable's name
  get_filename_component(TestFileName ${TestSource} NAME_WE)
  add_executable(${TestFileName} ${TestSource} ${CMAKE_SOURCE_DIR}/tests/unity/src/unity.c)

  # link against cthings framework
  target_link_libraries(${TestFileName} PRIVATE falcon uv ${llhttp_parser})
  target_include_directories(${TestFileName} PUBLIC ${CMAKE_SOURCE_DIR}/include  ${CMAKE_SOURCE_DIR}/tests/unity/src)

  add_test(NAME ${TestFileName} COMMAND ${TestFileName})
endforeach()
