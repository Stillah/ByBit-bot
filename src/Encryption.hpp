#pragma once
#include <iostream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <openssl/hmac.h>
#include <nlohmann/json.hpp>
#include "BotBase.h"

namespace Encryption {

std::string GeneratePostSignature(const nlohmann::json &parameters, const BotBase &bot, const std::string& Timestamp);
std::string GenerateGetSignature(const nlohmann::json &parameters, const BotBase &bot, const std::string& Timestamp);
std::string ComputeSignature(const std::string &data, const BotBase &bot);
std::string GenerateQueryString(const nlohmann::json &parameters);

std::string GeneratePostSignature(const nlohmann::json &parameters, const BotBase &bot, const std::string& Timestamp) {
  std::string paramJson = parameters.dump();
  std::string rawData = Timestamp + bot.api_key + bot.recvWindow + paramJson;
  return ComputeSignature(rawData, bot);
}

std::string GenerateGetSignature(const nlohmann::json &parameters, const BotBase &bot, const std::string& Timestamp) {
  std::string queryString = GenerateQueryString(parameters);
  std::string rawData = Timestamp + bot.api_key + bot.recvWindow + queryString;
  return ComputeSignature(rawData, bot);
}

std::string ComputeSignature(const std::string &data, const BotBase &bot) {
  unsigned char *digest = HMAC(EVP_sha256(), bot.secret_key.c_str(), static_cast<int>(bot.secret_key.length()),
                               (unsigned char *) data.c_str(), static_cast<int>(data.size()), nullptr, nullptr);

  std::ostringstream result;
  for (size_t i = 0; i < 32; i++) {
    result << std::hex << std::setw(2) << std::setfill('0') << (int) digest[i];
  }
  return result.str();
}

std::string GenerateQueryString(const nlohmann::json &parameters) {
  std::ostringstream oss;
  for (const auto &item: parameters.items()) {
    const std::string &key = item.key();
    std::string value = item.value().get<std::string>();
    oss << key << "=" << value << "&";
  }
  std::string result = oss.str();
  result.pop_back(); // Remove trailing '&'
  return result;
}

} //Encryption::
