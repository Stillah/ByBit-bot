#pragma once
#include <string>

struct BotBase {
  BotBase(const std::string&);
  BotBase() = delete;
  BotBase(const BotBase &) = delete;
  BotBase &operator=(const BotBase &) = delete;
  BotBase(BotBase &&) = delete;
  BotBase &operator=(BotBase &&) = delete;

  std::string api_key;
  std::string secret_key;
  std::string ticker;
  std::string host;
  std::string recvWindow;
  std::string port;
  double leverage;
};
