#pragma once
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <nlohmann/json.hpp>
#include "ByBit.h"
#include "BotBase.h"

using json = nlohmann::json;

void Strategy(const std::string &file) {
  ByBit bybit(file);
  try {
    if (bybit.is_socket_open()) {
      json subscription_message = {
        {"op", "subscribe"},
        {"args", {"wallet"}}
      };
      bybit.write_Socket(subscription_message.dump());
    }

    bybit.makeLongOrder();
    bybit.debug();

    while (true) {
      bybit.read_Socket();
      std::cout << bybit.get_socket_data() << std::endl;
      bybit.buffer_clear();
    }
    bybit.webSocket_close();
  } catch (std::exception &e) { std::cout << e.what() << "\n"; }
}
