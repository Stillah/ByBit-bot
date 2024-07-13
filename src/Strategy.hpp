#pragma once
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/strand.hpp>
#include <nlohmann/json.hpp>
#include "Encryption.hpp"
#include "BotBase.h"

namespace beast = boost::beast;     // from <boost/beast.hpp>
namespace http = beast::http;       // from <boost/beast/http.hpp>
namespace net = boost::asio;        // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;   // from <boost/asio/ssl.hpp>
using tcp = net::ip::tcp;           // from <boost/asio/ip/tcp.hpp>
using json = nlohmann::json;


//Setting up request fields that won't change in the future
http::request<http::string_body> initPostReq(const BotBase &bot) {

  http::request<http::string_body> req{http::verb::post, "/v5/order/create", 11};
  req.set(http::field::host, bot.host);
  req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

  req.set("X-BAPI-API-KEY", bot.api_key);
  req.set("X-BAPI-SIGN-TYPE", "2");
  req.set("X-BAPI-RECV-WINDOW", bot.recvWindow);
  req.set(http::field::content_type, "application/json");
  return req;
}

void makeLongOrder(http::request<http::string_body> &req, const BotBase &bot, json &makeLongOrderParams,
               beast::ssl_stream<beast::tcp_stream> &stream) {
  std::string Timestamp = std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>
                                           (std::chrono::system_clock::now().time_since_epoch()).count());
  //change makeLongOrderParams, mainly price
  
  req.set("X-BAPI-SIGN", Encryption::GeneratePostSignature(makeLongOrderParams, bot, Timestamp));
  req.set("X-BAPI-TIMESTAMP", Timestamp);
  req.body() = makeLongOrderParams.dump();
  req.prepare_payload();
  http::write(stream, req);

  //std::cout << req << "\n";
}

void Strategy(const BotBase &bot) {
  try {
    net::io_context ioc;
    ssl::context ctx(ssl::context::tlsv12_client);
    tcp::resolver resolver(ioc);
    beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx);

    // Set SNI Hostname (many hosts need this to handshake successfully)
    if (!SSL_set_tlsext_host_name(stream.native_handle(), bot.host.c_str())) {
      beast::error_code ec{static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()};
      throw beast::system_error{ec};
    }

    auto const results = resolver.resolve(bot.host, bot.port);
    beast::get_lowest_layer(stream).connect(results);
    stream.handshake(ssl::stream_base::client);
    beast::flat_buffer buffer;
    http::response<http::dynamic_body> res;

    json makeLongOrderParams{
      {"category", "spot"},
      {"symbol", bot.ticker},
      {"side", "Buy"},
      {"positionIdx", 0},
      {"orderType", "Limit"},
      {"qty", "0.001"},
      {"price", "57000"},
      {"timeInForce", "GTC"}
    };
    // Sample order
    auto orderReq = initPostReq(bot);
    makeLongOrder(orderReq, bot, makeLongOrderParams, stream);

    http::read(stream, buffer, res);

    auto ans = json::parse(beast::buffers_to_string(res.body().cdata()));
    std::cout << ans.dump(2) << std::endl;


    // Gracefully close the stream
    beast::error_code ec;
    stream.shutdown(ec);
    if (ec == net::error::eof) ec = {};
    if (ec) throw beast::system_error{ec};
    // If we get here then the connection is closed gracefully
  }
  catch (std::exception const &e) {
    std::cerr << "Error: " << e.what() << std::endl;
  }
}
