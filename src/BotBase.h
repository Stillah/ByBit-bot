#pragma once
#include <string>

struct BotBase {
  explicit BotBase(const std::string&);
  BotBase() = default;
  BotBase(const BotBase &) = default;
  BotBase &operator=(const BotBase &) = default;
  BotBase(BotBase &&) = default;
  BotBase &operator=(BotBase &&) = default;

  std::string api_key, secret_key, ticker, host, webSocketPrivate,
  webSocketPublic, recvWindow, port, leverage;
  double quantity, takeProfit, stopLoss;
  int32_t updatePriceInterval, syncOrdersInterval;
};
