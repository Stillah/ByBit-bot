#pragma once
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <fstream>
#include <nlohmann/json.hpp>
#include "ByBit.h"
#include "BotBase.h"

#define currTimeMS duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count()

using json = nlohmann::json;

inline void updateOrderLinkId(std::string &linkOrderId, int8_t offset) noexcept {
  linkOrderId.erase(offset);
  linkOrderId += std::to_string(currTimeMS);
}

bool checkWsSubscription(ByBit &exchange) {
  exchange.read_private_Socket();
  return json::parse(exchange.get_socket_data())["success"];
}

// TODO: make async_ping with boost
void Strategy(const std::string &file) {
  int32_t takeProfits = 0, stopLoses = 0, totalRequests = 0;
  int64_t pingTime = currTimeMS;
  ByBit bybit(file);
  //io_ping.run();
  std::ofstream output("output.txt");
  std::string longOrderId = "l-" + bybit.getTickerName();
  std::string shortOrderId = "s-" + bybit.getTickerName();
  int8_t offset = longOrderId.size();
  if (!bybit.socket_is_opened()) {
    std::cerr << "Couldn't open websocket.\n";
    return;
  }

  //Subscribe to websocket stream
  json subscription_message{{"op", "subscribe"}, {"args", {"execution"}}};
  bybit.write_private_Socket(subscription_message.dump());

  updateOrderLinkId(longOrderId, offset);
  updateOrderLinkId(shortOrderId, offset);
  bybit.placeBoth(bybit.getTickerPrice(), longOrderId, shortOrderId);

  for (;;) {
    try { bybit.read_private_Socket(); }
    catch (std::exception &e) {
      std::string message = e.what();
      if (message == "End of file [asio.misc:2]") {
        std::cerr << "EOF " << currTimeMS << "\n";
        continue;
      }
      std::cerr << e.what() << " " << currTimeMS - pingTime << "\n";
      break;
    }
    auto response = json::parse(bybit.get_socket_data());
    output << response.dump(2) << "\n";
    if (response["data"][0]["stopOrderType"] == "TakeProfit") {
      if (response["data"][0]["side"] == "Sell") {
        std::cerr << "Long takeprofit triggered. TP/SL: " << ++takeProfits << "/" << stopLoses << "\n";
        updateOrderLinkId(longOrderId, offset);
        auto BidAsk = bybit.getTickerPrice();
        bybit.placeLong(BidAsk.first, longOrderId);
        bybit.changeOrder(BidAsk.second, shortOrderId);
        ++totalRequests;
      }
      else if (response["data"][0]["side"] == "Buy") {
        std::cerr << "Short takeprofit triggered. TP/SL: " << ++takeProfits << "/" << stopLoses << "\n";
        updateOrderLinkId(shortOrderId, offset);
        auto BidAsk = bybit.getTickerPrice();
        bybit.placeShort(BidAsk.second, shortOrderId);
        bybit.changeOrder(BidAsk.first, longOrderId);
        ++totalRequests;
      }
    }
    else if (response["data"][0]["stopOrderType"] == "StopLoss") {
      if (response["data"][0]["side"] == "Sell") {
        std::cerr << "Long stoploss triggered. TP/SL: " << takeProfits << "/" << ++stopLoses << "\n";
        updateOrderLinkId(longOrderId, offset);
        auto BidAsk = bybit.getTickerPrice();
        bybit.placeLong(BidAsk.first, longOrderId);
        bybit.changeOrder(BidAsk.second, shortOrderId);
        ++totalRequests;
      }
      else if (response["data"][0]["side"] == "Buy") {
        std::cerr << "Short stoploss triggered. TP/SL: " << takeProfits << "/" << ++stopLoses << "\n";
        updateOrderLinkId(shortOrderId, offset);
        auto BidAsk = bybit.getTickerPrice();
        bybit.placeShort(BidAsk.second, shortOrderId);
        bybit.changeOrder(BidAsk.first, longOrderId);
        ++totalRequests;
      }
    }
    if (pingTime + 20000 < currTimeMS) {
      bybit.sendPing();
      pingTime = currTimeMS;
    }
    bybit.buffer_clear();
  }
  std::cerr << "Total zayavok: " << totalRequests << "\n";
}
