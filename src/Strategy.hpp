#pragma once
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <fstream>
#include <thread>
#include <nlohmann/json.hpp>
#include "ByBit.h"
#include "BotBase.h"

using json = nlohmann::json;

inline void updateOrderLinkId(std::string &linkOrderId, int8_t offset) noexcept {
  linkOrderId.erase(offset);
  linkOrderId += std::to_string(currTimeMS);
}

bool checkWsSubscription(ByBit &exchange) {
  bool opened = false, subscribed = false;
  exchange.read_private_Socket();
  opened = static_cast<bool>(json::parse(exchange.get_socket_data())["success"]);
  exchange.buffer_clear();

  exchange.read_private_Socket();
  subscribed = static_cast<bool>(json::parse(exchange.get_socket_data())["success"]);
  exchange.buffer_clear();
  return (opened && subscribed);
}

void Strategy(const std::string &file) {
  int32_t takeProfits = 0, stopLoses = 0, totalRequests = 0;
  std::ofstream output("output.txt");
  std::ifstream input(file, std::ios::in);
  std::string ticker;
  try {
    if (!file.contains(".json")) throw std::invalid_argument("File needs to be .json");
    json parameters = json::parse(input);
    ticker = parameters["ticker"];
    input.close();
  }
  catch (std::exception &e) {
    std::cerr << e.what() << std::endl;
    assert(false);
  }
  std::string longOrderId = "l-" + ticker;
  std::string shortOrderId = "s-" + ticker;
  int8_t offset = longOrderId.size();
  ByBit bybit(file, longOrderId, shortOrderId);

  auto callback = [&](beast::error_code ec, size_t) {
    if (ec) {
      std::cerr << "(callback) " << ec.message() << "\n";
      return;
    }

    auto response = json::parse(bybit.get_socket_data());
    //std::output << response.dump(2) << "\n";

    if (response["data"][0]["stopOrderType"] == "TakeProfit") {
      if (response["data"][0]["side"] == "Sell") {
        std::cerr << "Long takeprofit triggered. TP/SL: " << ++takeProfits << "/" << stopLoses << "\n";
        updateOrderLinkId(longOrderId, offset);
        auto BidAsk = bybit.getTickerPrice();
        bybit.placeLong(BidAsk.first);
        ++totalRequests;
        bybit.longNotFilled = true;
      }
      else if (response["data"][0]["side"] == "Buy") {
        std::cerr << "Short takeprofit triggered. TP/SL: " << ++takeProfits << "/" << stopLoses << "\n";
        updateOrderLinkId(shortOrderId, offset);
        auto BidAsk = bybit.getTickerPrice();
        bybit.placeShort(BidAsk.second);
        ++totalRequests;
        bybit.shortNotFilled = true;
      }
    }
    else if (response["data"][0]["stopOrderType"] == "StopLoss") {
      if (response["data"][0]["side"] == "Sell") {
        std::cerr << "Long stoploss triggered. TP/SL: " << takeProfits << "/" << ++stopLoses << "\n";
        updateOrderLinkId(longOrderId, offset);
        auto BidAsk = bybit.getTickerPrice();
        bybit.placeLong(BidAsk.first);
        ++totalRequests;
        bybit.longNotFilled = true;
      }
      else if (response["data"][0]["side"] == "Buy") {
        std::cerr << "Short stoploss triggered. TP/SL: " << takeProfits << "/" << ++stopLoses << "\n";
        updateOrderLinkId(shortOrderId, offset);
        auto BidAsk = bybit.getTickerPrice();
        bybit.placeShort(BidAsk.second);
        ++totalRequests;
        bybit.shortNotFilled = true;
      }
    }
    else if (response["data"][0]["isMaker"] == true) {
      if (static_cast<std::string>(response["data"][0]["orderLinkId"])[0] == 's')
        bybit.shortNotFilled = false;
      else
        bybit.longNotFilled = false;
    }
    bybit.buffer_clear();
    bybit.async_read_private_Socket();
  };

  //Subscribe to websocket stream
  json subscription_message{{"op", "subscribe"}, {"args", {"execution"}}};
  bybit.write_private_Socket(subscription_message.dump());
  if (!checkWsSubscription(bybit)) {
    std::cerr << "Couldn't open websocket or subscribe to it.\n";
    return;
  }
  //output << "Current USDT balance: " << bybit.getUSDTBalance() << "\n";
  bybit.setHedgingMode(true);
  updateOrderLinkId(longOrderId, offset);
  updateOrderLinkId(shortOrderId, offset);
  bybit.placeBoth(bybit.getTickerPrice());

  bybit.setCallback(callback);
  bybit.async_read_private_Socket();

  strategyBegin:
  try {
    bybit.ws_ioc_run();
  }
  // No idea what to actually do
  catch (std::exception &e) {
    std::cerr << "Hopefully everything's fine\n";
    std::cerr << e.what() << "\n";
    bybit.cancelOrders();
    updateOrderLinkId(longOrderId, offset);
    updateOrderLinkId(shortOrderId, offset);
    bybit.placeBoth(bybit.getTickerPrice());
    goto strategyBegin;
  }

  std::cerr << "Total executed orders: " << totalRequests << "\n";
  output.close();
}
