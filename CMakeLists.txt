cmake_minimum_required(VERSION 3.22)

project(group_work_stealing VERSION 1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_EXTENSIONS OFF)
find_package(TBB REQUIRED)
find_package(OpenMP REQUIRED)

# Add source files
set(SOURCES
    src/work_stealing_config.hpp
)
# set(Sources2
#     src/concepts.hpp
#     src/task.hpp
#     src/task_group.hpp
#     src/work_stealing_scheduler.hpp
#     src/taskManager/simpleTaskManager.hpp
# )
# Main library
add_library(group_work_stealing INTERFACE)
target_sources(group_work_stealing INTERFACE ${SOURCES})
target_include_directories(group_work_stealing INTERFACE src)
target_compile_features(group_work_stealing INTERFACE cxx_std_20)
target_link_libraries(group_work_stealing INTERFACE TBB::tbb TBB::tbbmalloc OpenMP::OpenMP_CXX)
# #second library
# # Main library
# add_library(group_work_stealing2 INTERFACE)
# target_sources(group_work_stealing2 INTERFACE ${SOURCES2})
# target_include_directories(group_work_stealing2 INTERFACE src)
# target_compile_features(group_work_stealing2 INTERFACE cxx_std_20)
# target_link_libraries(group_work_stealing2 INTERFACE TBB::tbb TBB::tbbmalloc OpenMP::OpenMP_CXX)

# tree generator
add_executable(binaryBalancedTreeGenerator tools/binaryBalancedTreeGenerator.cpp)
add_executable(unbalancesTreeGenerator tools/unbalancesTreeGenerator.cpp)

# Ensure the data directory exists
add_custom_target(create_data_dir
    COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_SOURCE_DIR}/data
    COMMENT "Ensuring data directory exists"
)
add_custom_command(
    OUTPUT
    ${PROJECT_SOURCE_DIR}/data/binaryBalancedTree.graph
    ${PROJECT_SOURCE_DIR}/data/4_aryBalancedTree.graph
    ${PROJECT_SOURCE_DIR}/data/bigger_6ary.graph
    ${PROJECT_SOURCE_DIR}/data/unbalencedTree.graph
    ${PROJECT_SOURCE_DIR}/data/unbalencedWeightedTree.graph
    COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_SOURCE_DIR}/data
    COMMAND binaryBalancedTreeGenerator 8 2 > ${PROJECT_SOURCE_DIR}/data/binaryBalancedTree.graph
    COMMAND binaryBalancedTreeGenerator 10 4 > ${PROJECT_SOURCE_DIR}/data/4_aryBalancedTree.graph
    COMMAND binaryBalancedTreeGenerator 12 6 > ${PROJECT_SOURCE_DIR}/data/bigger_6ary.graph
    COMMAND unbalancesTreeGenerator 12 6 > ${PROJECT_SOURCE_DIR}/data/unbalencedTree.graph
    COMMAND unbalancesTreeGenerator 12 6 10 > ${PROJECT_SOURCE_DIR}/data/unbalencedWeightedTree.graph
    DEPENDS binaryBalancedTreeGenerator unbalancesTreeGenerator create_data_dir
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMENT "Generating multiple benchmark data files"
    VERBATIM
)

add_custom_target(generateData DEPENDS
    ${PROJECT_SOURCE_DIR}/data/binaryBalancedTree.graph
    ${PROJECT_SOURCE_DIR}/data/4_aryBalancedTree.graph
    ${PROJECT_SOURCE_DIR}/data/bigger_6ary.graph
    ${PROJECT_SOURCE_DIR}/data/unbalencedTree.graph
    ${PROJECT_SOURCE_DIR}/data/unbalencedWeightedTree.graph
)


# add_executable(test_work_stealing_scheduler test/test_work_stealing_scheduler.cpp)
# target_link_libraries(test_work_stealing_scheduler PRIVATE group_work_stealing)
# target_link_libraries(test_work_stealing_scheduler INTERFACE TBB::tbb  TBB::tbbmalloc OpenMP::OpenMP_CXX)

if(PROJECT_IS_TOP_LEVEL)
    add_subdirectory(benchmark)
    add_subdirectory(test)
endif()
