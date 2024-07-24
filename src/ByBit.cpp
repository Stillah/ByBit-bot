#include "ByBit.h"
#include "Encryption.hpp"
#include <boost/format.hpp>


ByBit::ByBit(const std::string &file) {
  bot = BotBase(file);
  init_http();
  initPostReq();
  initGetReq();
  initCancelReq();
  //init_public_webSocket();
  init_private_webSocket();
  setLeverage();
  getInstrumentInfo();

  ordersParams["request"][0]["symbol"] = ordersParams["request"][1]["symbol"] = bot.ticker;
  ordersParams["request"][0]["qty"] = ordersParams["request"][1]["qty"] =
    (boost::format(qtyFmt) % (stod(bot.quantity) / 2)).str();
  cancelParams["request"][0]["symbol"] = cancelParams["request"][1]["symbol"] = bot.ticker;
}

ByBit::~ByBit() { cancelOrders(); }

void ByBit::read_private_Socket() { wsPrivate.read(buffer); }

void ByBit::read_public_Socket() { wsPublic.read(buffer); }

bool ByBit::socket_is_opened() { return wsPrivate.is_open() && wsPublic.is_open(); }

void ByBit::write_private_Socket(const std::string &text) { wsPrivate.write(net::buffer(text)); }

void ByBit::write_public_Socket(const std::string &text) { wsPublic.write(net::buffer(text)); }

void ByBit::webSocket_close() {
  if (wsPrivate.is_open()) wsPrivate.close(websocket::close_code::none);
  if (wsPublic.is_open()) wsPublic.close(websocket::close_code::none);
}

std::string ByBit::get_socket_data() { return beast::buffers_to_string(buffer.data()); }

void ByBit::cancelOrders() {
  std::string Timestamp = std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>
                                           (std::chrono::system_clock::now().time_since_epoch()).count());
  cancelRequest.set("X-BAPI-TIMESTAMP", Timestamp);
  cancelRequest.set("X-BAPI-SIGN", Encryption::GeneratePostSignature(cancelParams, bot, Timestamp));
  cancelRequest.body() = cancelParams.dump();
  cancelRequest.prepare_payload();

  http::write(stream, cancelRequest);
  http::read(stream, buffer, res);
  res.body().clear();
}

void ByBit::placeOrders(double price) {
  // change ordersParams, mainly price and TP/SL
  ordersParams["request"][0]["price"] = (boost::format(priceFmt) % (price - delta)).str();
  ordersParams["request"][0]["takeProfit"] = (boost::format(priceFmt) % (price + 2000 * delta)).str();
  ordersParams["request"][0]["stopLoss"] = (boost::format(priceFmt) % (price - 4000 * delta)).str();

  ordersParams["request"][1]["price"] = (boost::format(priceFmt) % (price + delta)).str();
  ordersParams["request"][1]["takeProfit"] = (boost::format(priceFmt) % (price - 2000 * delta)).str();
  ordersParams["request"][1]["stopLoss"] = (boost::format(priceFmt) % (price + 4000 * delta)).str();

  std::string Timestamp = std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>
                                           (std::chrono::system_clock::now().time_since_epoch()).count());
  orderRequest.set("X-BAPI-TIMESTAMP", Timestamp);
  orderRequest.set("X-BAPI-SIGN", Encryption::GeneratePostSignature(ordersParams, bot, Timestamp));
  orderRequest.body() = ordersParams.dump();
  orderRequest.prepare_payload();

  http::write(stream, orderRequest);
  http::read(stream, buffer, res);
  //res.body().clear();
}

void ByBit::setLeverage() {
  http::request<http::string_body> req{http::verb::post, "/v5/position/set-leverage", 11};
  json leverage = {
    {"category", "linear"},
    {"symbol", bot.ticker},
    {"buyLeverage", bot.leverage},
    {"sellLeverage", bot.leverage}
  };

  req.set(http::field::host, bot.host);
  req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
  req.set("X-BAPI-API-KEY", bot.api_key);
  req.set("X-BAPI-SIGN-TYPE", "2");
  req.set("X-BAPI-RECV-WINDOW", bot.recvWindow);
  req.set(http::field::content_type, "application/json");

  std::string Timestamp = std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>
                                           (std::chrono::system_clock::now().time_since_epoch()).count());

  req.set("X-BAPI-TIMESTAMP", Timestamp);
  req.set("X-BAPI-SIGN", Encryption::GeneratePostSignature(leverage, bot, Timestamp));
  req.body() = leverage.dump();
  req.prepare_payload();

  http::write(stream, req);
  http::read(stream, buffer, res);
  json answer = json::parse(beast::buffers_to_string(res.body().data()));
  if (answer["retCode"] != 0 && answer["retMsg"] != "leverage not modified")
    std::cout << req << "\n" << answer << "\n";

  res.body().clear();
  buffer.clear();
}

//returns a pair of bidPrice and askPrice
std::pair<double, double> ByBit::getTickerPrice() {
  http::write(stream, getRequest);
  http::read(stream, buffer, res);
  auto response = json::parse(beast::buffers_to_string(res.body().cdata()));
  res.body().clear();
  return {stod(static_cast<std::string>(response["result"]["list"][0]["bid1Price"])),
          stod(static_cast<std::string>(response["result"]["list"][0]["ask1Price"]))};
}

void ByBit::getInstrumentInfo() {
  http::request<http::string_body> req{http::verb::get, "/v5/market/instruments-info?category=linear&symbol=" +
    bot.ticker, 11};
  req.set(http::field::host, bot.host);
  req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
  req.set(http::field::content_type, "application/json");
  http::write(stream, req);
  http::read(stream, buffer, res);

  auto response = json::parse(beast::buffers_to_string(res.body().cdata()));
  res.body().clear();

  int8_t quantityScale = 0;
  bool flag = false;
  std::string qtyStep = response["result"]["list"][0]["lotSizeFilter"]["qtyStep"];
  for (char c: qtyStep) {
    quantityScale += flag;
    if (c == '.') flag = true;
  }
  //формат цены и количества (цифры после запятой), шаг цены, fees(?)
  priceFmt = std::format("%1.{0}f", static_cast<std::string>(response["result"]["list"][0]["priceScale"]));
  qtyFmt = std::format("%1.{0}f", quantityScale);
  delta = stod(static_cast<std::string>(response["result"]["list"][0]["priceFilter"]["tickSize"]));
}

void ByBit::debug() {
  //std::cout << orderRequest << "\n";

  try {
    auto response = json::parse(beast::buffers_to_string(res.body().cdata()));
    std::cout << response.dump(2) << std::endl;
  }
  catch (...) { std::cout << beast::buffers_to_string(res.body().cdata()) << "\n"; }
}

void ByBit::buffer_clear() { buffer.clear(); }

void ByBit::authenticate() {
  int64_t expires = std::chrono::duration_cast<std::chrono::milliseconds>(
    std::chrono::system_clock::now().time_since_epoch()).count() + 1000;

  std::string val = "GET/realtime" + std::to_string(expires);
  std::string signature = Encryption::ComputeSignature(val, bot);

  json auth_msg = {
    {"op", "auth"},
    {"args", {bot.api_key, expires, signature}}
  };

  write_private_Socket(auth_msg.dump());
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
}

void ByBit::init_private_webSocket() {
  // Set SNI Hostname (many hosts need this to handshake successfully)
  if (!SSL_set_tlsext_host_name(wsPrivate.next_layer().native_handle(), bot.webSocketPrivate.c_str()))
    throw beast::system_error(
      beast::error_code(static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()),
      "Failed to set SNI Hostname");

  auto const results = resolver_webSocket.resolve(bot.webSocketPrivate, bot.port);
  net::connect(wsPrivate.next_layer().next_layer(), results.begin(), results.end());
  wsPrivate.next_layer().handshake(ssl::stream_base::client);
  wsPrivate.handshake(bot.webSocketPrivate, "/v5/private");
  authenticate();
}

void ByBit::init_public_webSocket() {
  // Set SNI Hostname (many hosts need this to handshake successfully)
  if (!SSL_set_tlsext_host_name(wsPublic.next_layer().native_handle(), bot.webSocketPublic.c_str()))
    throw beast::system_error(
      beast::error_code(static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()),
      "Failed to set SNI Hostname");

  auto const results = resolver_webSocket.resolve(bot.webSocketPublic, bot.port);
  net::connect(wsPublic.next_layer().next_layer(), results.begin(), results.end());
  wsPublic.next_layer().handshake(ssl::stream_base::client);
  wsPublic.handshake(bot.webSocketPublic, "/v5/public/spot");
}

void ByBit::initPostReq() {
  http::request<http::string_body> req{http::verb::post, "/v5/order/create-batch", 11};
  req.set(http::field::host, bot.host);
  req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

  req.set("X-BAPI-API-KEY", bot.api_key);
  req.set("X-BAPI-SIGN-TYPE", "2");
  req.set("X-BAPI-RECV-WINDOW", bot.recvWindow);
  req.set(http::field::content_type, "application/json");
  orderRequest = req;
}

void ByBit::initGetReq() {
  http::request<http::string_body> req{http::verb::get, "/v5/market/tickers?category=linear&symbol=" + bot.ticker, 11};
  req.set(http::field::host, bot.host);
  req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
  req.set(http::field::content_type, "application/json");
  getRequest = req;
}

void ByBit::initCancelReq() {
  cancelParams["symbol"] = bot.ticker;
  http::request<http::string_body> req{http::verb::post, "/v5/order/cancel-all", 11};
  req.set(http::field::host, bot.host);
  req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

  req.set("X-BAPI-API-KEY", bot.api_key);
  req.set("X-BAPI-SIGN-TYPE", "2");
  req.set("X-BAPI-RECV-WINDOW", bot.recvWindow);
  req.set(http::field::content_type, "application/json");
  cancelRequest = req;
}

