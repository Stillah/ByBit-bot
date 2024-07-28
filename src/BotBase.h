#pragma once
#include <string>

struct BotBase {
  BotBase(const std::string&);
  BotBase() = default;
  BotBase(const BotBase &) = default;
  BotBase &operator=(const BotBase &) = default;
  BotBase(BotBase &&) = default;
  BotBase &operator=(BotBase &&) = default;

  std::string api_key, secret_key, ticker, host, webSocketPrivate,
  webSocketPublic, recvWindow, port, quantity, leverage;
  double takeProfit, stopLoss;
};
