#include "BotBase.h"
#include "Strategy.hpp"

int main() {
  BotBase bot("parameters.json");
  Strategy(bot);

  return 0;
}
