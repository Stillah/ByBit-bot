#pragma once
#include <iostream>
#include "Encryption.hpp"
#include "BotBase.h"

void initEncryption(const BotBase& bot) {
//  Encryption::secret_key = bot.secret_key;
//  Encryption::api_key = bot.api_key;
//  Encryption::recvWindow = bot.recvWindow;
}


void Strategy(const BotBase& bot) {
  initEncryption(bot);
  std::cout << "a";
}