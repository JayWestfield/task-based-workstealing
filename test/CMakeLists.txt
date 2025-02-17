cmake_minimum_required(VERSION 3.22)

# Add test executable
add_executable(tests test_work_stealing_scheduler.cpp)
target_compile_features(tests PRIVATE cxx_std_20)
target_link_libraries(tests PRIVATE group_work_stealing)
