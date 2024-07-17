#include "ByBit.h"
#include "Encryption.hpp"

ByBit::ByBit(const std::string &file) {
  bot = BotBase(file);
  init_http();
  init_webSocket();
  authenticate();
  makeLongOrderParams["symbol"] = bot.ticker;
  //Add other parameters like leverage
}

void ByBit::read_Socket() { ws.read(buffer); }

bool ByBit::is_socket_open() { return ws.is_open(); }

void ByBit::write_Socket(const std::string &text) { ws.write(net::buffer(text)); }

std::string ByBit::get_socket_data() { return beast::buffers_to_string(buffer.data()); }

void ByBit::buffer_clear() { buffer.clear(); }

void ByBit::webSocket_close() { ws.close(websocket::close_code::none); }

void ByBit::authenticate() {
  long long int expires = std::chrono::duration_cast<std::chrono::milliseconds>(
    std::chrono::system_clock::now().time_since_epoch()).count() + 1000;

  std::string val = "GET/realtime" + std::to_string(expires);

  std::string signature = Encryption::ComputeSignature(val, bot);

  json auth_msg = {
    {"op", "auth"},
    {"args", {bot.api_key, expires, signature}}
  };

  write_Socket(auth_msg.dump());
}

void ByBit::makeLongOrder() {
  std::string Timestamp = std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>
                                           (std::chrono::system_clock::now().time_since_epoch()).count());
  //change makeLongOrderParams, mainly price

  orderRequest.set("X-BAPI-SIGN", Encryption::GeneratePostSignature(makeLongOrderParams, bot, Timestamp));
  orderRequest.set("X-BAPI-TIMESTAMP", Timestamp);
  orderRequest.body() = makeLongOrderParams.dump();
  orderRequest.prepare_payload();
  http::write(stream, orderRequest);
}

void ByBit::debug() {
  std::cout << orderRequest << "\n";
  http::read(stream, buffer, res);

  auto ans = json::parse(beast::buffers_to_string(res.body().cdata()));
  std::cout << ans.dump(2) << std::endl;
}

void ByBit::init_http() {
  // Set SNI Hostname (many hosts need this to handshake successfully)
  if (!SSL_set_tlsext_host_name(stream.native_handle(), bot.host.c_str())) {
    boost::system::error_code ec{static_cast<int>(::ERR_get_error()), boost::asio::error::get_ssl_category()};
    throw boost::system::system_error{ec};
  }
  const auto results{resolver.resolve(bot.host, bot.port)};
  get_lowest_layer(stream).connect(results);
  stream.handshake(ssl::stream_base::client);
  initPostReq();
}

void ByBit::init_webSocket() {
  // Set SNI Hostname (many hosts need this to handshake successfully)
  if (!SSL_set_tlsext_host_name(ws.next_layer().native_handle(), bot.webSocket.c_str()))
    throw beast::system_error(
      beast::error_code(static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()),
      "Failed to set SNI Hostname");

  auto const results = resolver_webSocket.resolve(bot.webSocket, bot.port);
  net::connect(ws.next_layer().next_layer(), results.begin(), results.end());
  ws.next_layer().handshake(ssl::stream_base::client);
  ws.handshake(bot.webSocket, "/v5/private");
}

void ByBit::initPostReq() {

  http::request<http::string_body> req{http::verb::post, "/v5/order/create", 11};
  req.set(http::field::host, bot.host);
  req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

  req.set("X-BAPI-API-KEY", bot.api_key);
  req.set("X-BAPI-SIGN-TYPE", "2");
  req.set("X-BAPI-RECV-WINDOW", bot.recvWindow);
  req.set(http::field::content_type, "application/json");
  orderRequest = req;
}

