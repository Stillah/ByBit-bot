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

  BotBase bot;
  explicit ByBit(const std::string &);
  ~ByBit();

  void read_private_Socket();
  void read_public_Socket();
  bool socket_is_opened();
  void write_private_Socket(const std::string &);
  void write_public_Socket(const std::string &);
  void webSocket_close();
  std::string get_socket_data();

  void cancelOrders();
  void placeOrders(double);
  void setLeverage();
  std::pair<double, double> getTickerPrice();
  void getInstrumentInfo();
  void debug();
  void buffer_clear();

  private:

  void init_http();
  void init_private_webSocket();
  void init_public_webSocket();
  void initPostReq();
  void authenticate();
  void initGetReq();
  void initCancelReq();

  double delta;
  std::string priceFmt{"0:.2f"}, qtyFmt{"0:.3f"};

  net::io_context ioc, ioc_webSocket;
  ssl::context ctx{ssl::context::tlsv12_client}, ctx_webSocket{ssl::context::tlsv12_client};
  tcp::resolver resolver{ioc}, resolver_webSocket{ioc_webSocket};
  Stream stream{ioc, ctx};
  beast::flat_buffer buffer;
  http::response<http::dynamic_body> res;
  http::request<http::string_body> orderRequest, getRequest, cancelRequest;
  websocket::stream<beast::ssl_stream<tcp::socket>> wsPublic{ioc_webSocket, ctx_webSocket},
    wsPrivate{ioc_webSocket, ctx_webSocket};

  json cancelParams{{"category", "linear"}};
  json ordersParams{
    {"category", "linear"},
    {
      "request", {
      {{"side", "Buy"},
       {"orderType", "Limit"},
       {"isLeverage", 1},
       {"positionIdx", 1},
       {"tpTriggerBy", "LastPrice"},
       {"slTriggerBy", "LastPrice"}},

      {{"side", "Sell"},
       {"orderType", "Limit"},
       {"isLeverage", 1},
       {"positionIdx", 2},
       {"tpTriggerBy", "LastPrice"},
       {"slTriggerBy", "LastPrice"}}
    }
    }
  };
};



