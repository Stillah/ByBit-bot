#pragma once
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/high_resolution_timer.hpp>
#include <nlohmann/json.hpp>
#include <fstream>
#include "BotBase.h"
#define currTimeMS duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count()

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
  //bool shortNotFilled = true, longNotFilled = true;

  ByBit(const std::string &, std::ofstream &, std::string &, std::string &);
  ByBit() = delete;
  ByBit(const ByBit &) = delete;
  ByBit &operator=(const ByBit &) = delete;
  ByBit(ByBit &&) = delete;
  ByBit &operator=(ByBit &&) = delete;
  ~ByBit();

  void read_private_Socket();
  void ws_ioc_run();
  void read_public_Socket();
  void async_read_private_Socket();
  bool socket_is_opened();
  void write_private_Socket(const std::string &);
  void write_public_Socket(const std::string &);
  void webSocket_close();
  std::string get_socket_data() const noexcept;
  void setCallback(const std::function<void(beast::error_code, size_t)> &);

  void cancelOrders();
  void placeBoth(std::pair<long double, long double>);
  void placeLong(long double);
  void placeShort(long double);
  void changeBoth(std::pair<long double, long double>);
  void changeLong(long double);
  void changeShort(long double);
  void setLeverage();
  void setHedgingMode(bool on);
  std::pair<long double, long double> getTickerPrice();
  void getInstrumentInfo();
  std::string getUSDTBalance();
  std::pair<bool, bool> ordersAreActive();

  void debug(const http::request<http::string_body> &, const http::response<http::dynamic_body> &);
  void buffer_clear() noexcept;
  void consume_buffer(size_t) noexcept;
  inline std::string priceToStr(long double) noexcept;
  inline std::string qtyToStr(long double) noexcept;
  std::string getTickerName() const noexcept;

  private:

  void authenticate();
  void init_http();
  void init_private_webSocket();
  void init_public_webSocket();
  http::request<http::string_body> getBasePrivateReq(const std::string &, http::verb) const;
  http::request<http::string_body> getBasePublicReq(const std::string &, http::verb) const;
  inline void preparePrivateReq(const json &, http::request<http::string_body> &) const;
  void beginAsyncPings(const boost::system::error_code &ec);
  void chasePrice(const boost::system::error_code &ec);
  void initOrderReq();
  void initPriceReq();
  void initCancelReq();
  void initAmendRequest();

  BotBase bot;
  long double delta, takeProfit, stopLoss;
  std::string priceFmt, qtyFmt;
  std::string &longLinkId, &shortLinkId;
  std::ofstream &output;
  std::function<void(beast::error_code, size_t)> callback;

  net::io_context ioc, ioc_webSocket;
  net::steady_timer m_timer;
  net::high_resolution_timer chaseTimer;
  ssl::context ctx{ssl::context::tlsv12_client}, ctx_webSocket{ssl::context::tlsv12_client};
  tcp::resolver resolver{ioc}, resolver_webSocket{ioc_webSocket};
  Stream stream{ioc, ctx};
  beast::flat_buffer buffer, wsBuffer, priceBuffer;
  http::response<http::dynamic_body> cancelRes, orderRes, changeRes, priceRes, res;
  http::request<http::string_body> orderRequest, priceRequest, cancelRequest, amendRequest;
  websocket::stream<beast::ssl_stream<tcp::socket>> wsPublic{ioc_webSocket, ctx_webSocket},
    wsPrivate{ioc_webSocket, ctx_webSocket};

  json cancelParams = {{"category", "linear"},
                       {"orderFilter", "Order"}};
  json orderParams = {{"category", "linear"},
                      {"request", {}}};
  json amendParams = {{"category", "linear"},
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
