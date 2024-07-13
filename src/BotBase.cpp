#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include "BotBase.h"
using json = nlohmann::json;

BotBase::BotBase(const std::string& fileName) {
  std::ifstream file(fileName, std::ios::in);
  try {
    if (!fileName.contains(".json")) throw std::invalid_argument("File needs to be .json");
    json parameters = json::parse(file);
    api_key = parameters["api_key"];
    secret_key = parameters["secret_key"];
    ticker = parameters["ticker"];
    host = parameters["host"];
    recvWindow = parameters["recvWindow"];
    leverage = parameters["leverage"];
    port = parameters["port"];
    file.close();
  } catch(std::exception& e) {
    std::cerr << e.what()<< std::endl;
    assert(false);
  }
}