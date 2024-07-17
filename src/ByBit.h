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

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>

using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
using Request = http::request<http::string_body>;
using Stream = beast::ssl_stream<beast::tcp_stream>;
using json = nlohmann::json;

class ByBit {
  public:
  explicit ByBit(const std::string &);

  void read_Socket();
  bool is_socket_open();
  void write_Socket(const std::string &text);
  std::string get_socket_data();
  void buffer_clear();
  void webSocket_close();
  void makeLongOrder();
  void debug();

  private:
  BotBase bot;
  net::io_context ioc;
  ssl::context ctx{ssl::context::tlsv12_client};
  tcp::resolver resolver{ioc};
  Stream stream{ioc, ctx};
  http::response<http::dynamic_body> res;
  http::request<http::string_body> orderRequest;

  beast::flat_buffer buffer;
  net::io_context ioc_webSocket;
  ssl::context ctx_webSocket{ssl::context::tlsv12_client};
  tcp::resolver resolver_webSocket{ioc_webSocket};
  websocket::stream<beast::ssl_stream<tcp::socket>> ws{ioc_webSocket,
                                                       ctx_webSocket};

  json makeLongOrderParams{
    {"category", "spot"},
    {"side", "Buy"},
    {"positionIdx", 0},
    {"orderType", "Limit"},
    {"qty", "0.001"},
    {"price", "59500"},
    {"timeInForce", "GTC"}
  };

  void init_http();
  void init_webSocket();
  void initPostReq();
  void authenticate();
};



