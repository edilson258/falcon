add_subdirectory(gtest)

include(GoogleTest)

set(tests_sources
        ${CMAKE_CURRENT_SOURCE_DIR}/test_simple.cpp
)

foreach (test_source ${tests_sources})
    get_filename_component(test_name ${test_source} NAME_WE)
    add_executable(${test_name} ${test_source})
    target_include_directories(${test_name}
            PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/gtest/googletest/include)
    target_link_libraries(
            ${test_name}
            falcon
            gtest_main
    )
    gtest_discover_tests(${test_name})
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach ()
