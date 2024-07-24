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
  std::string webSocketPrivate;
  std::string webSocketPublic;
  std::string recvWindow;
  std::string port;
  std::string quantity;
  std::string leverage;
};
