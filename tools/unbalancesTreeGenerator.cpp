#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <sstream>
#include <vector>

void generateUnbalancedTree(int depth, int maxChildren, int maxWeight = 1) {
  std::srand(depth * maxChildren * maxWeight);
  int currentDepth = 0;
  int nodeCount = 0;
  std::vector<int> nodesAtCurrentDepth = {0};
  std::ostringstream output;

  while (currentDepth < depth) {
    std::vector<int> nodesAtNextDepth;
    for (int node : nodesAtCurrentDepth) {
      int children =
          std::rand() % (maxChildren + 1); // Allow 0 to maxChildren children
      for (int j = 0; j < children; ++j) {
        ++nodeCount;
        auto weight = (std::rand() % (maxWeight)) + 1;
        output << node << " " << nodeCount << " " << weight << std::endl;
        nodesAtNextDepth.push_back(nodeCount);
      }
    }
    if (nodesAtNextDepth.empty() && currentDepth < depth - 1) {
      // Ensure at least one node reaches the max depth
      ++nodeCount;
      auto weight = (std::rand() % (maxWeight)) + 1;
      output << nodesAtCurrentDepth[0] << " " << nodeCount << " " << weight
             << std::endl;
      nodesAtNextDepth.push_back(nodeCount);
    }
    nodesAtCurrentDepth = nodesAtNextDepth;
    ++currentDepth;
  }

  std::cout << nodeCount + 1 << std::endl; // Including the root node
  std::cout << output.str();
}

int main(int argc, char *argv[]) {
  if (argc > 4) {
    std::cerr << "Usage: " << argv[0] << " <depth> <maxChildren> [<maxWeight>]"
              << std::endl;
    return 1;
  }

  int depth = std::stoi(argv[1]);
  int maxChildren = std::stoi(argv[2]);
  int maxWeight = (argc == 4) ? std::stoi(argv[3]) : 1;

  generateUnbalancedTree(depth, maxChildren, maxWeight);

  return 0;
}