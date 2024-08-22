#pragma once
#include <functional>
#include <iostream>
#include <string>
#include <fstream>
#include <nlohmann/json.hpp>
#include "ByBit.h"

using json = nlohmann::json;

bool checkWsSubscription(ByBit &exchange, std::ofstream &output) {
  bool opened = false, subscribed = false;

  exchange.read_private_Socket();
  json openMessage = json::parse(exchange.get_socket_data());
  opened = static_cast<bool>(openMessage["success"]);
  exchange.buffer_clear();

  exchange.read_private_Socket();
  json subscribeMessage = json::parse(exchange.get_socket_data());
  subscribed = static_cast<bool>(subscribeMessage["success"]);
  exchange.buffer_clear();

  if (!opened)
    output << "Couldn't open websocket\n" << openMessage.dump(2) << std::endl;
  if (!subscribed)
    output << "Couldn't subscribe to websocket\n" << subscribeMessage.dump(2) << std::endl;

  return (opened && subscribed);
}

void Strategy(const std::string &config, const std::string &log, int32_t &takeProfits, int32_t &stopLoses,
              long double &fees, long double &generatedVolume) {
  int8_t offset;
  std::ifstream input(config, std::ios::in);
  std::ofstream output(log, std::ios::app);
  std::string longOrderId, shortOrderId;

  try {
    if (!config.contains(".json")) throw std::invalid_argument("File needs to be .json");
    json parameters = json::parse(input);
    longOrderId = "l-" + static_cast<std::string>(parameters["ticker"]);
    shortOrderId = "s-" + static_cast<std::string>(parameters["ticker"]);
    offset = longOrderId.size();
    input.close();
  }
  catch (std::exception &e) {
    output << e.what() << std::endl;
    output << "[" << currDateAndTime() << "]" << std::endl;
    assert(false);
  }

  ByBit bybit(config, output, longOrderId, shortOrderId);

  auto main_callback = [&](beast::error_code ec, size_t messageSize) {
    if (ec) {
      output << "(main_callback) " << ec.message() << std::endl;
      throw std::exception();
    }

    auto response = json::parse(bybit.get_socket_data());

    if (response["data"][0]["stopOrderType"] == "TakeProfit") {
      if (response["data"][0]["side"] == "Sell") {
        output << "Long";
        updateOrderLinkId(longOrderId, offset);
        auto BidAsk = bybit.getTickerPrice();
        bybit.placeLong(BidAsk.first);
      }
      else {
        output << "Short";
        updateOrderLinkId(shortOrderId, offset);
        auto BidAsk = bybit.getTickerPrice();
        bybit.placeShort(BidAsk.second);
      }
      generatedVolume += stod(response["data"][0]["execValue"].get<std::string>());
      fees += stod(response["data"][0]["execFee"].get<std::string>());
      output << " takeprofit triggered | TP/SL: " << ++takeProfits << "/" << stopLoses << " | Total Fees: "
             << fees << " | Generated volume: " << generatedVolume << std::endl;
    }
    else if (response["data"][0]["stopOrderType"] == "StopLoss") {
      if (response["data"][0]["side"] == "Sell") {
        output << "Long";
        updateOrderLinkId(longOrderId, offset);
        auto BidAsk = bybit.getTickerPrice();
        bybit.placeLong(BidAsk.first);
      }
      else {
        output << "Short";
        updateOrderLinkId(shortOrderId, offset);
        auto BidAsk = bybit.getTickerPrice();
        bybit.placeShort(BidAsk.second);
      }
      generatedVolume += stod(response["data"][0]["execValue"].get<std::string>());
      fees += stod(response["data"][0]["execFee"].get<std::string>());
      output << " stoploss triggered | TP/SL: " << takeProfits << "/" << ++stopLoses << " | Total Fees: "
             << fees << " | Generated volume: " << generatedVolume << std::endl;
    }
//    else if (response["data"][0]["isMaker"].get<bool>()) {
//      if (static_cast<std::string>(response["data"][0]["orderLinkId"])[0] == 's')
//        bybit.shortNotFilled = false;
//      else
//        bybit.longNotFilled = false;
//    }
    bybit.consume_buffer(messageSize);
    bybit.async_read_private_Socket();
  };

  auto sync_callback = [&](beast::error_code ec, size_t messageSize) {
    if (ec) {
      output << "(sync_callback) " << ec.message() << std::endl;
      throw std::exception();
    }

    auto response = json::parse(bybit.get_socket_data());

    if (response["data"][0]["stopOrderType"] == "TakeProfit") {
      if (response["data"][0]["side"] == "Sell")
        output << "Long";
      else
        output << "Short";

      generatedVolume += stod(response["data"][0]["execValue"].get<std::string>());
      fees += stod(response["data"][0]["execFee"].get<std::string>());
      output << " takeprofit triggered | TP/SL: " << ++takeProfits << "/" << stopLoses << " | Total Fees: "
             << fees << " | Generated volume: " << generatedVolume << std::endl;
      output << orderExecuted + 1 << "/2 orders executed" << std::endl;

      if (orderExecuted) {
        updateOrderLinkId(longOrderId, offset);
        updateOrderLinkId(shortOrderId, offset);
        bybit.placeBoth(bybit.getTickerPrice());
        bybit.resetSyncTimer();
        needToSync = false;
      }
      orderExecuted = !orderExecuted;
    }
    else if (response["data"][0]["stopOrderType"] == "StopLoss") {
      if (response["data"][0]["side"] == "Sell")
        output << "Long";
      else
        output << "Short";

      generatedVolume += stod(response["data"][0]["execValue"].get<std::string>());
      fees += stod(response["data"][0]["execFee"].get<std::string>());
      output << " stoploss triggered | TP/SL: " << takeProfits << "/" << ++stopLoses << " | Total Fees: "
             << fees << " | Generated volume: " << generatedVolume << std::endl;
      output << orderExecuted + 1 << "/2 orders executed" << std::endl;

      if (orderExecuted) {
        updateOrderLinkId(longOrderId, offset);
        updateOrderLinkId(shortOrderId, offset);
        bybit.placeBoth(bybit.getTickerPrice());
        bybit.resetSyncTimer();
        needToSync = false;
      }
      orderExecuted = !orderExecuted;
    }
    bybit.consume_buffer(messageSize);
    bybit.async_read_private_Socket();
  };

  //Subscribe to websocket stream
  json subscription_message{{"op", "subscribe"}, {"args", {"execution"}}};
  bybit.write_private_Socket(subscription_message.dump());
  if (!checkWsSubscription(bybit, output))
    assert(false);

  updateOrderLinkId(longOrderId, offset);
  updateOrderLinkId(shortOrderId, offset);
  output << "Starting USDT balance: " << bybit.getUSDTBalance() << std::endl;
  bybit.setHedgingMode(true);
  bybit.setCallbacks(main_callback, sync_callback);
  bybit.cancelOrders();

  if (!needToSync) {
    auto activeOrders = bybit.ordersAreActive();
    if (!activeOrders.first && !activeOrders.second)
      bybit.placeBoth(bybit.getTickerPrice());
    else if (!activeOrders.first)
      bybit.placeLong(bybit.getTickerPrice().first);
    else if (!activeOrders.second)
      bybit.placeShort(bybit.getTickerPrice().second);
  }
  bybit.async_read_private_Socket();

  try { bybit.ws_ioc_run(); }
  catch (std::exception &e) {
    output << "Something went wrong: " << e.what() << std::endl;
    output << "[" << currDateAndTime() << "]" << std::endl;
    output.close();
  }
}
