add_executable(example example.cpp)
target_link_libraries(example falcon)
target_compile_options(example PRIVATE
        $<$<CONFIG:Debug>:-O0 -g -fsanitize=address -fsanitize=undefined>
)
target_link_options(example PRIVATE
        $<$<CONFIG:Debug>:-fsanitize=address -fsanitize=undefined>
)
