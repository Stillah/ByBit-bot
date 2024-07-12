#include "BotBase.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
using json = nlohmann::json;

BotBase::BotBase() {
  std::ifstream file("parameters.json", std::ios::in);
  try {
    json parameters = json::parse(file);
    api_key = parameters["api_key"];
    secret_key = parameters["secret_key"];
    ticker = parameters["ticker"];
    host = parameters["host"];
    recvWindow = parameters["recvWindow"];
    leverage = parameters["leverage"];
    file.close();
  } catch(std::exception& e) {
    std::cerr << e.what()<< std::endl;
    assert(false);
  }
}