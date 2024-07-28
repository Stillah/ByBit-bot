#include "ByBit.h"
#include "Encryption.hpp"
#include <boost/format.hpp>


ByBit::ByBit(const std::string &file) {
  std::cerr << "Bot started construction\n";
  bot = BotBase(file);
  init_http();
  initOrderReq();
  initPriceReq();
  initCancelReq();
  initAmendRequest();
  //init_public_webSocket();
  init_private_webSocket();
  setLeverage();
  getInstrumentInfo();

  cancelParams["request"][0]["symbol"] = cancelParams["request"][1]["symbol"] = bot.ticker;
  longPosParams["symbol"] = shortPosParams["symbol"] = bot.ticker;
  longPosParams["qty"] = shortPosParams["qty"] = priceToStr(stod(bot.quantity) / 2);
  takeProfit = bot.takeProfit;
  stopLoss = bot.stopLoss;
  //m_timer.async_wait([this](const boost::system::error_code &ec) { timerExpired(ec); });

  std::cerr << "Bot finished construction\n";
}

ByBit::~ByBit() {
  std::cerr << "Cancelling orders...\n";
  cancelOrders();
  webSocket_close();
  std::cerr << "Orders cancelled\n";
}

void ByBit::read_private_Socket() { wsPrivate.read(wsBuffer); }

void ByBit::read_public_Socket() { wsPublic.read(wsBuffer); }

bool ByBit::socket_is_opened() { return wsPrivate.is_open() /*&& wsPublic.is_open()*/; }

void ByBit::write_private_Socket(const std::string &text) { wsPrivate.write(net::buffer(text)); }

void ByBit::write_public_Socket(const std::string &text) { wsPublic.write(net::buffer(text)); }

void ByBit::webSocket_close() {
  if (wsPrivate.is_open()) wsPrivate.close(websocket::close_code::none);
  if (wsPublic.is_open()) wsPublic.close(websocket::close_code::none);
}

std::string ByBit::get_socket_data() const noexcept { return beast::buffers_to_string(wsBuffer.data()); }

void ByBit::sendPing() {
  write_private_Socket(R"({"op":"ping"})");
  std::cerr << "Ping sent\n";
}

void ByBit::cancelOrders() {
  preparePostReq(cancelParams, cancelRequest);
  http::write(stream, cancelRequest);
  http::read(stream, buffer, cancelRes);
  cancelRes.body().clear();
}

//Maybe remove assert and ifs to lower latency
void ByBit::placeBoth(std::pair<double, double> price, const std::string &longLinkId, const std::string &shortLinkId) {
  auto [bidPrice, askPrice] = price;

  orderParams["request"][0] = longPosParams;
  orderParams["request"][0]["price"] = priceToStr(bidPrice - delta);
  orderParams["request"][0]["takeProfit"] = priceToStr(bidPrice + bidPrice * takeProfit);
  orderParams["request"][0]["stopLoss"] = priceToStr(bidPrice - bidPrice * stopLoss);
  orderParams["request"][0]["orderLinkId"] = longLinkId;

  orderParams["request"][1] = shortPosParams;
  orderParams["request"][1]["price"] = priceToStr(askPrice + delta);
  orderParams["request"][1]["takeProfit"] = priceToStr(askPrice - askPrice * takeProfit);
  orderParams["request"][1]["stopLoss"] = priceToStr(askPrice + askPrice * stopLoss);
  orderParams["request"][1]["orderLinkId"] = shortLinkId;

  preparePostReq(orderParams, orderRequest);
  http::write(stream, orderRequest);
  http::read(stream, buffer, orderRes);
  orderRes.body().clear();
  //debug();
  orderParams["request"].clear();
  std::cerr << "Both orders placed\n";
}

void ByBit::placeLong(double bidPrice, const std::string &longLinkId) {
  orderParams["request"][0] = longPosParams;
  orderParams["request"][0]["price"] = priceToStr(bidPrice - delta);
  orderParams["request"][0]["takeProfit"] = priceToStr(bidPrice + bidPrice * takeProfit);
  orderParams["request"][0]["stopLoss"] = priceToStr(bidPrice - bidPrice * stopLoss);
  orderParams["request"][0]["orderLinkId"] = longLinkId;

  preparePostReq(orderParams, orderRequest);
  http::write(stream, orderRequest);
  http::read(stream, buffer, orderRes);
  orderRes.body().clear();
  //debug();
  orderParams["request"].clear();
  std::cerr << "Long order placed\n";
}

void ByBit::placeShort(double askPrice, const std::string &shortLinkId) {
  orderParams["request"][0] = shortPosParams;
  orderParams["request"][0]["price"] = priceToStr(askPrice + delta);
  orderParams["request"][0]["takeProfit"] = priceToStr(askPrice - askPrice * takeProfit);
  orderParams["request"][0]["stopLoss"] = priceToStr(askPrice + askPrice * stopLoss);
  orderParams["request"][0]["orderLinkId"] = shortLinkId;

  preparePostReq(orderParams, orderRequest);
  http::write(stream, orderRequest);
  http::read(stream, buffer, orderRes);
  orderRes.body().clear();
  //debug();
  orderParams["request"].clear();
  std::cerr << "Short order placed\n";
}

void ByBit::changeOrder(double price, const std::string &orderLinkId) {
  amendParams["orderLinkId"] = orderLinkId;
  amendParams["price"] = priceToStr(price);
  amendParams["qty"] = priceToStr(stod(bot.quantity));
  amendParams["takeProfit"] = priceToStr(price + price * (orderLinkId[0] == 'l' ? takeProfit : -takeProfit));
  amendParams["stopLoss"] = priceToStr(price - price * (orderLinkId[0] == 'l' ? stopLoss : -stopLoss));

  preparePostReq(amendParams, amendRequest);
  http::write(stream, amendRequest);
  http::read(stream, buffer, changeRes);
  //debug();
  changeRes.body().clear();
  std::cerr << (orderLinkId[0] == 'l' ? "Long " : "Short ") << "order changed\n";
}

void ByBit::setLeverage() {
  json leverage = {
    {"category", "linear"},
    {"symbol", bot.ticker},
    {"buyLeverage", bot.leverage},
    {"sellLeverage", bot.leverage}
  };
  auto req = getBasePostReq("/v5/position/set-leverage");

  preparePostReq(leverage, req);
  http::write(stream, req);
  http::read(stream, buffer, res);

  json answer = json::parse(beast::buffers_to_string(res.body().data()));
  if (answer["retCode"] != 0 && answer["retMsg"] != "leverage not modified")
    std::cerr << req << "\n" << answer << "\n";

  res.body().clear();

}

//returns a pair of bidPrice and askPrice
std::pair<double, double> ByBit::getTickerPrice() {
  http::write(stream, priceRequest);
  http::read(stream, buffer, res);
  auto response = json::parse(beast::buffers_to_string(res.body().cdata()));
  res.body().clear();
  return {stod(static_cast<std::string>(response["result"]["list"][0]["bid1Price"])),
          stod(static_cast<std::string>(response["result"]["list"][0]["ask1Price"]))};
}

void ByBit::getInstrumentInfo() {
  auto req = getBaseGetReq("/v5/market/instruments-info?category=linear&symbol=" + bot.ticker);
  http::write(stream, req);
  http::read(stream, buffer, res);

  auto response = json::parse(beast::buffers_to_string(res.body().cdata()));
  res.body().clear();

  int8_t quantityScale = 0;
  bool flag = false;
  std::string qtyStep = response["result"]["list"][0]["lotSizeFilter"]["qtyStep"];
  // digits after . in qty
  for (char c: qtyStep) {
    quantityScale += flag;
    if (c == '.') flag = true;
  }
  // формат цены и количества (цифры после запятой), шаг цены
  priceFmt = std::format("%1.{0}f", static_cast<std::string>(response["result"]["list"][0]["priceScale"]));
  qtyFmt = std::format("%1.{0}f", quantityScale);
  delta = stod(static_cast<std::string>(response["result"]["list"][0]["priceFilter"]["tickSize"]));
}

void ByBit::debug() {
  //std::cerr << amendRequest << "\n";

  try {
    auto response = json::parse(beast::buffers_to_string(changeRes.body().cdata()));
    std::cerr << response.dump(2) << std::endl;
  }
  catch (...) { std::cerr << beast::buffers_to_string(changeRes.body().cdata()) << "\n"; }
  changeRes.body().clear();
}

void ByBit::buffer_clear() noexcept { wsBuffer.clear(); }

std::string ByBit::getTickerName() const noexcept { return bot.ticker; }

inline std::string ByBit::priceToStr(double price) noexcept {
  return (boost::format(priceFmt) % price).str();
}

void ByBit::authenticate() {
  int64_t expires = std::chrono::duration_cast<std::chrono::milliseconds>(
    std::chrono::system_clock::now().time_since_epoch()).count() + 1000;

  std::string val = "GET/realtime" + std::to_string(expires);
  std::string signature = Encryption::ComputeSignature(val, bot);

  const json auth_msg = {
    {"op", "auth"},
    {"args", {bot.api_key, expires, signature}}
  };

  write_private_Socket(auth_msg.dump());
}

void ByBit::init_http() {
  // Set SNI Hostname (many hosts need this to handshake successfully)
  if (!SSL_set_tlsext_host_name(stream.native_handle(), bot.host.c_str())) {
    boost::system::error_code ec{static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()};
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

[[nodiscard]] http::request<http::string_body> ByBit::getBasePostReq(const std::string &endpoint) const {
  http::request<http::string_body> req{http::verb::post, endpoint, 11};

  req.set(http::field::host, bot.host);
  req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
  req.set("X-BAPI-API-KEY", bot.api_key);
  req.set("X-BAPI-SIGN-TYPE", "2");
  req.set("X-BAPI-RECV-WINDOW", bot.recvWindow);
  req.set(http::field::content_type, "application/json");
  return req;
}

[[nodiscard]] http::request<http::string_body> ByBit::getBaseGetReq(const std::string &endpoint) const {
  http::request<http::string_body> req{http::verb::get, endpoint, 11};

  req.set(http::field::host, bot.host);
  req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
  req.set(http::field::content_type, "application/json");
  return req;
}

inline void ByBit::preparePostReq(const json &data, http::request<http::string_body> &req) const {
  std::string Timestamp = std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>
                                           (std::chrono::system_clock::now().time_since_epoch()).count());
  req.set("X-BAPI-TIMESTAMP", Timestamp);
  req.set("X-BAPI-SIGN", Encryption::GeneratePostSignature(data, bot, Timestamp));
  req.body() = data.dump();
  req.prepare_payload();
}

//void ByBit::timerExpired(const boost::system::error_code &ec) {
//  if (ec) {
//    std::cerr << ec << "\n";
//    return;
//  }
//
//  std::cerr << "Ping sent\n";
//  wsPrivate.ping(R"("op":"ping")");
//  m_timer.expires_after(net::chrono::seconds(20));
//  m_timer.async_wait([this](const boost::system::error_code &ec) { timerExpired(ec); });
//}

void ByBit::initOrderReq() {
  orderRequest = getBasePostReq("/v5/order/create-batch");
}

void ByBit::initPriceReq() {
  priceRequest = getBaseGetReq("/v5/market/tickers?category=linear&symbol=" + bot.ticker);
}

void ByBit::initCancelReq() {
  cancelParams["symbol"] = bot.ticker;
  cancelRequest = getBasePostReq("/v5/order/cancel-all");
}

void ByBit::initAmendRequest() {
  amendParams["symbol"] = bot.ticker;
  amendRequest = getBasePostReq("/v5/order/amend");
}
