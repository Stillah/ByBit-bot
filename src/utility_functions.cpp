#include "utility_functions.h"

bool needToSync = false;
bool orderExecuted = false;

void updateOrderLinkId(std::string &linkOrderId, int8_t offset) noexcept {
  linkOrderId.erase(offset);
  linkOrderId += std::to_string(currTimeMS);
}

std::string currDateAndTime() noexcept {
  std::time_t t = std::time(nullptr);
  char buffer[64];
  strftime(buffer, sizeof(buffer), "%Y-%h-%d %H_%M_%S", std::localtime(&t));
  return buffer;
}

std::string Encryption::GeneratePostSignature(const nlohmann::json &parameters, const BotBase &bot, const std::string
&Timestamp) {
  std::string paramJson = parameters.dump();
  std::string rawData = Timestamp + bot.api_key + bot.recvWindow + paramJson;
  return ComputeSignature(rawData, bot);
}

std::string
Encryption::GenerateGetSignature(const nlohmann::json &parameters, const BotBase &bot, const std::string &Timestamp) {
  std::string queryString = GenerateQueryString(parameters);
  std::string rawData = Timestamp + bot.api_key + bot.recvWindow + queryString;
  return ComputeSignature(rawData, bot);
}

std::string Encryption::ComputeSignature(const std::string &data, const BotBase &bot) {
  unsigned char *digest = HMAC(EVP_sha256(), bot.secret_key.c_str(), static_cast<int>(bot.secret_key.length()),
                               (unsigned char *) data.c_str(), static_cast<int>(data.size()), nullptr, nullptr);

  std::ostringstream result;
  for (size_t i = 0; i < 32; i++)
    result << std::hex << std::setw(2) << std::setfill('0') << (int) digest[i];
  return result.str();
}

std::string Encryption::GenerateQueryString(const nlohmann::json &parameters) {
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
