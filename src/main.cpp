#include "BotBase.h"
#include "Strategy.hpp"

int main() {
  std::ios_base::sync_with_stdio(false); std::cout.tie(nullptr); std::cin.tie(nullptr);
  Strategy("parameters.json");

  return 0;
}
