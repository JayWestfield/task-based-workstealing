cmake_minimum_required(VERSION 3.22)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "")
set(BENCHMARK_ENABLE_WERROR OFF)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "")
set(BENCHMARK_ENABLE_GTEST_TESTS OFF)
add_subdirectory(../external/googlebenchmark googlebenchmark EXCLUDE_FROM_ALL)

# Benchmark
# add_executable(benchmark_balanced_tree benchmark_balanced_tree.cpp)
# target_compile_features(benchmark_balanced_tree PRIVATE cxx_std_20)
# target_compile_options(benchmark_balanced_tree PRIVATE -O3 -g -march=native -DNDEBUG)
# target_link_libraries(benchmark_balanced_tree PRIVATE group_work_stealing)

# add_executable(benchmark_unbalanced_tree benchmark_unbalanced_tree.cpp)
# target_compile_features(benchmark_unbalanced_tree PRIVATE cxx_std_20)
# target_compile_options(benchmark_unbalanced_tree PRIVATE -O3 -g -march=native -DNDEBUG)
# target_link_libraries(benchmark_unbalanced_tree PRIVATE group_work_stealing)

# add_executable(benchmark_weighted_tree benchmark_weighted_tree.cpp)
# target_compile_features(benchmark_weighted_tree PRIVATE cxx_std_20)
# target_compile_options(benchmark_weighted_tree PRIVATE -O3 -g -march=native -DNDEBUG)
# target_link_libraries(benchmark_weighted_tree PRIVATE group_work_stealing)

add_executable(benchmark_base benchmark_base.cpp)
target_compile_features(benchmark_base PRIVATE cxx_std_20)
target_compile_options(benchmark_base PRIVATE -O3 -g -march=native -DNDEBUG)
target_link_libraries(benchmark_base PRIVATE group_work_stealing benchmark::benchmark)

# Plots
add_custom_command(OUTPUT benchmark_balanced_tree.json
  COMMAND benchmark_base
  ARGS
  "${PROJECT_SOURCE_DIR}/data/binaryBalancedTree.graph"
  "0"
  --benchmark_out=benchmark_balanced_tree.json --benchmark_out_format=json

  DEPENDS benchmark_base
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
  COMMENT "Running balanced tree benchmark"
  VERBATIM)

add_custom_command(OUTPUT benchmark_balanced_4_arytree.json
  COMMAND benchmark_base
  ARGS
  "${PROJECT_SOURCE_DIR}/data/4_aryBalancedTree.graph"
  "0"
  --benchmark_out=benchmark_balanced_4_arytree.json --benchmark_out_format=json

  DEPENDS benchmark_base
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
  COMMENT "Running 4-arytree balanced tree benchmark"
  VERBATIM)
add_custom_command(OUTPUT benchmark_bigger_6ary.json
  COMMAND benchmark_base
  ARGS
  "${PROJECT_SOURCE_DIR}/data/bigger_6ary.graph"
  "0"
  --benchmark_out=benchmark_bigger_6ary.json --benchmark_out_format=json

  DEPENDS benchmark_base
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
  COMMENT "Running bigger_6ary balanced tree benchmark"
  VERBATIM)

add_custom_command(OUTPUT benchmark_unbalanced_tree.json
  COMMAND benchmark_base
  ARGS
  "${PROJECT_SOURCE_DIR}/data/unbalencedTree.graph"
  "5"
  --benchmark_out=benchmark_unbalanced_tree.json --benchmark_out_format=json
  DEPENDS benchmark_base
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
  COMMENT "Running unbalanced tree benchmark"
  VERBATIM)

add_custom_command(OUTPUT benchmark_weighted_tree.json
  COMMAND benchmark_base
  ARGS
  "${PROJECT_SOURCE_DIR}/data/test_tree.graph"
  "0"
  --benchmark_out=benchmark_weighted_tree.json --benchmark_out_format=json
  DEPENDS benchmark_base
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
  COMMENT "Running weighted tree benchmark"
  VERBATIM)

add_custom_command(OUTPUT plots.pdf
  COMMAND Rscript "${PROJECT_SOURCE_DIR}/eval/make_plots.R"
  DEPENDS "${PROJECT_SOURCE_DIR}/eval/make_plots.R"
  benchmark_unbalanced_tree.json #benchmark_bigger_6ary.json  benchmark_balanced_tree.json  benchmark_balanced_4_arytree.json   # n benchmark_weighted_tree.json
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
  COMMENT "Generating plots"
  VERBATIM)

add_custom_target(plots DEPENDS plots.pdf)

# custom allocations try


# add_custom_target(bench DEPENDS benchmark_balanced_tree.json)

# Profiler
add_executable(profile_work_stealing_scheduler profile_work_stealing_scheduler.cpp)
target_compile_features(profile_work_stealing_scheduler PRIVATE cxx_std_20)
target_compile_options(profile_work_stealing_scheduler PRIVATE -O3 -g -march=native -DNDEBUG)
target_link_libraries(profile_work_stealing_scheduler PRIVATE group_work_stealing tcmalloc profiler pthread benchmark::benchmark)

# Custom command to run profiler, transform results, and open with kcachegrind
add_custom_command(
  OUTPUT profiling_results/transformed_gperf.out
  COMMAND profile_work_stealing_scheduler ${PROJECT_SOURCE_DIR}/data/unbalencedTree.graph 10 12
  COMMAND google-pprof --callgrind profile_work_stealing_scheduler work_stealing_scheduler.prof > transformed_gperf.out
  COMMAND kcachegrind transformed_gperf.out
  DEPENDS profile_work_stealing_scheduler
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMENT "Running profiler, transforming results, and opening with kcachegrind"
  VERBATIM
)

add_custom_target(run_profiler DEPENDS profiling_results/transformed_gperf.out)
