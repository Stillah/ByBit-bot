#pragma once
#include <iostream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <openssl/hmac.h>
#include <nlohmann/json.hpp>
#include "BotBase.h"
#define currTimeMS duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count()

extern bool needToSync, orderExecuted;

void updateOrderLinkId(std::string &linkOrderId, int8_t offset) noexcept;

std::string currDateAndTime() noexcept;

namespace Encryption {

std::string GeneratePostSignature(const nlohmann::json &parameters, const BotBase &bot, const std::string &Timestamp);
std::string GenerateGetSignature(const nlohmann::json &parameters, const BotBase &bot, const std::string &Timestamp);
std::string ComputeSignature(const std::string &data, const BotBase &bot);
std::string GenerateQueryString(const nlohmann::json &parameters);

} //Encryption::
