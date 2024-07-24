#pragma once
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <nlohmann/json.hpp>
#include "ByBit.h"
#include "BotBase.h"

#define currTime duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count()
using json = nlohmann::json;

// TODO: make async_ping with boost
void Strategy(const std::string &file) {
  ByBit bybit(file);
  try {
    if (!bybit.socket_is_opened()) return;

    json subscription_message{ {"op", "subscribe"}, {"args", {"order.linear"}} };
    bybit.write_private_Socket(subscription_message.dump());

    bybit.placeOrders(bybit.getTickerPrice().first);
    bybit.debug();
    bybit.cancelOrders();

    for (;;) {
      bybit.read_private_Socket();  // blocking call
      std::cout << json::parse(bybit.get_socket_data()).dump(2) << std::endl;
      bybit.buffer_clear();
    }
  }
  catch (std::exception &e) { std::cout << e.what() << "\n"; }
}
