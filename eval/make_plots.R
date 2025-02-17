library(tidyverse)
library(scales)
library(jsonlite)

generate_plot_speedup <- function(json_file, output_file) {
  # Read and process the JSON file
  data <- as_tibble(fromJSON(file(json_file))$benchmarks) %>%
    transmute(name = name, time = real_time) %>%
    separate(name, into = c("name", "threads"), sep = '/') %>%
    mutate(name = fct_relabel(factor(name), ~ gsub("^BM_", "", .)), threads = as.numeric(threads))
  
  # Debug: Print the processed data
  # print(data)
  
  # Calculate the speedup
  min_single_thread_time <- data %>% filter(threads == 1) %>% summarise(min_time = min(time)) %>% pull(min_time)
  data <- data %>% mutate(speedup = min_single_thread_time / time)
  
  # Plot: Comparison between different benchmarks
  p1 <- data %>%
    ggplot(aes(x = threads, y = speedup, color = name, shape = name)) +
    geom_line() +
    geom_point() +
    scale_x_continuous("Number of Threads") +
    scale_y_continuous("Speedup") +
    labs(title = paste("Benchmark Results -", json_file))
  
  # Save the plot
  ggsave(output_file, plot = p1, width = 10, height = 6)
}
generate_plot_time <- function(json_file, output_file) {
  # Read and process the JSON file
  data <- as_tibble(fromJSON(file(json_file))$benchmarks) %>%
    transmute(name = name, time = real_time) %>%
    separate(name, into = c("name", "threads"), sep = '/') %>%
    mutate(name = fct_relabel(factor(name), ~ gsub("^BM_", "", .)), threads = as.numeric(threads))
  
  # Debug: Print the processed data
  # print(data)
   
  # Plot: Comparison between different benchmarks
  p1 <- data %>%
    ggplot(aes(x = threads, y = time, color = name, shape = name)) +
    geom_line() +
    geom_point() +
    scale_x_continuous("Number of Threads") +
    scale_y_continuous("runtime") +
    labs(title = paste("Benchmark Results -", json_file))
  
  # Save the plot
  ggsave(output_file, plot = p1, width = 10, height = 6)
}
# Generate plots for each JSON file
# generate_plot_speedup("benchmark_balanced_tree.json", "balanced_tree_plot_speedup.pdf")
# generate_plot_time("benchmark_balanced_tree.json", "balanced_tree_plot_time.pdf")

generate_plot_speedup("benchmark_unbalanced_tree.json", "benchmark_unbalanced_tree_speedup.pdf")
generate_plot_time("benchmark_unbalanced_tree.json", "benchmark_unbalanced_tree_time.pdf")

generate_plot_speedup("benchmark_bigger_6ary.json", "benchmark_bigger_6ary_plot_speedup.pdf")
generate_plot_time("benchmark_bigger_6ary.json", "benchmark_bigger_6ary_plot_time.pdf")

generate_plot_speedup("benchmark_balanced_4_arytree.json", "benchmark_balanced_4_arytree_plot_speedup.pdf")
generate_plot_time("benchmark_balanced_4_arytree.json", "benchmark_balanced_4_arytree_plot_time.pdf")
# generate_plot("benchmark_unbalanced_tree.json", "unbalanced_tree_plot.pdf")
# generate_plot("benchmark_weighted_tree.json", "weighted_tree_plot.pdf")