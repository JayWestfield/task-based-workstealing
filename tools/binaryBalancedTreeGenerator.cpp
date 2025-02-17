#include <cmath>
#include <iostream>
#include <vector>

void generateBalancedTree(int depth, int children) {
  int maxNodes = (std::pow(children, depth + 1) - 1) / (children - 1);
  std::cout << maxNodes << std::endl;

  for (int i = 0; i < maxNodes; ++i) {
    for (int j = 1; j <= children; ++j) {
      int child = children * i + j;
      if (child < maxNodes) {
        std::cout << i << " " << child << " 1" << std::endl;
      }
    }
  }
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    std::cerr << "Usage: " << argv[0] << " <depth> <children>" << std::endl;
    return 1;
  }

  int depth = std::stoi(argv[1]);
  int children = std::stoi(argv[2]);
  generateBalancedTree(depth, children);

  return 0;
}