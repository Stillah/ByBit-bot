#pragma once
#include <string>

struct BotBase {
  BotBase();
  BotBase(const BotBase &) = delete;
  BotBase &operator=(const BotBase &) = delete;
  BotBase(BotBase &&) = delete;
  BotBase &operator=(BotBase &&) = delete;

  std::string api_key;
  std::string secret_key;
  std::string ticker;
  std::string host;
  std::string recvWindow;
  double leverage;
};
