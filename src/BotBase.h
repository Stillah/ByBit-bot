#pragma once
#include <string>

struct BotBase {
  BotBase(const std::string&);
  BotBase() = default;
  BotBase(const BotBase &) = default;
  BotBase &operator=(const BotBase &) = default;
  BotBase(BotBase &&) = default;
  BotBase &operator=(BotBase &&) = default;

  std::string api_key;
  std::string secret_key;
  std::string ticker;
  std::string host;
  std::string webSocket;
  std::string recvWindow;
  std::string port;
  double leverage;
};
