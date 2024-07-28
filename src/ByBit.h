#pragma once
#include <cstdlib>
#include <iostream>
#include <string>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/strand.hpp>
#include <nlohmann/json.hpp>
#include "BotBase.h"

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
namespace ssl = boost::asio::ssl;

using tcp = boost::asio::ip::tcp;
using Request = http::request<http::string_body>;
using Stream = beast::ssl_stream<beast::tcp_stream>;
using json = nlohmann::json;

//TODO: add noexcept and const
class ByBit {
  public:

  explicit ByBit(const std::string &);
  ByBit() = delete;
  ByBit(const ByBit &) = delete;
  ByBit &operator=(const ByBit &) = delete;
  ByBit(ByBit &&) = delete;
  ByBit &operator=(ByBit &&) = delete;
  ~ByBit();

  void read_private_Socket();
  void read_public_Socket();
  bool socket_is_opened();
  void write_private_Socket(const std::string &);
  void write_public_Socket(const std::string &);
  void webSocket_close();
  std::string get_socket_data() const noexcept;
  void sendPing();

  void cancelOrders();
  void placeBoth(std::pair<double, double>, const std::string &, const std::string &);
  void placeLong(double, const std::string &);
  void placeShort(double, const std::string &);
  void changeOrder(double, const std::string &);
  void setLeverage();
  std::pair<double, double> getTickerPrice();
  void getInstrumentInfo();

  void debug();
  void buffer_clear() noexcept;
  inline std::string priceToStr(double price) noexcept;
  std::string getTickerName() const noexcept;
  void init_private_webSocket();

  private:

  void authenticate();
  void init_http();

  void init_public_webSocket();
  [[nodiscard]] http::request<http::string_body> getBasePostReq(const std::string &) const;
  [[nodiscard]] http::request<http::string_body> getBaseGetReq(const std::string &) const;
  inline void preparePostReq(const json &, http::request<http::string_body> &) const;
  //void timerExpired(const boost::system::error_code &ec);
  void initOrderReq();
  void initPriceReq();
  void initCancelReq();
  void initAmendRequest();

  BotBase bot;
  double delta, takeProfit, stopLoss;
  std::string priceFmt, qtyFmt;

  net::io_context ioc, ioc_webSocket;
  //net::steady_timer m_timer;
  ssl::context ctx{ssl::context::tlsv12_client}, ctx_webSocket{ssl::context::tlsv12_client};
  tcp::resolver resolver{ioc}, resolver_webSocket{ioc_webSocket};
  Stream stream{ioc, ctx};
  beast::flat_buffer buffer, wsBuffer;
  http::response<http::dynamic_body> cancelRes, orderRes, changeRes, res;
  http::request<http::string_body> orderRequest, priceRequest, cancelRequest, amendRequest;
  websocket::stream<beast::ssl_stream<tcp::socket>> wsPublic{ioc_webSocket, ctx_webSocket},
    wsPrivate{ioc_webSocket, ctx_webSocket};

  json amendParams = {{"category", "linear"}};
  json cancelParams = {{"category", "linear"},
                       {"orderFilter", "Order"}};
  json orderParams = {{"category", "linear"},
                      {"request", {}}};

  json longPosParams = {{"side", "Buy"},
                        {"orderType", "Limit"},
                        {"isLeverage", 1},
                        {"positionIdx", 1},
                        {"tpTriggerBy", "LastPrice"},
                        {"slTriggerBy", "LastPrice"}};

  json shortPosParams = {{"side", "Sell"},
                         {"orderType", "Limit"},
                         {"isLeverage", 1},
                         {"positionIdx", 2},
                         {"tpTriggerBy", "LastPrice"},
                         {"slTriggerBy", "LastPrice"}};
};
