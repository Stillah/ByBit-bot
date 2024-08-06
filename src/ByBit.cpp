#include "ByBit.h"
#include "Encryption.hpp"
#include <boost/format.hpp>


ByBit::ByBit(const std::string &file, std::ofstream &out, std::string &longLinkIdRef, std::string &shortLinkIdRef)
  : longLinkId(longLinkIdRef),
    shortLinkId(shortLinkIdRef),
    output(out),
    m_timer(ioc_webSocket),
    chaseTimer(ioc_webSocket) {
  output << "Bot started construction" << std::endl;
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
  longPosParams["qty"] = shortPosParams["qty"] = qtyToStr(bot.quantity / (2 * getTickerPrice().first));
  takeProfit = bot.takeProfit / 100;
  stopLoss = bot.stopLoss / 100;
  output << "Bot finished construction" << std::endl;
}

ByBit::~ByBit() {
  webSocket_close();
//  wsPrivate.next_layer().shutdown();
//  wsPrivate.next_layer().next_layer().shutdown(net::socket_base::shutdown_type::shutdown_both);
//  stream.shutdown();
}

void ByBit::async_read_private_Socket() {
  wsPrivate.async_read(wsBuffer, callback);
}

void ByBit::read_private_Socket() {
  wsPrivate.read(wsBuffer);
}

void ByBit::ws_ioc_run() {
  m_timer.async_wait([this](const boost::system::error_code &ec) { beginAsyncPings(ec); });
  chaseTimer.async_wait([this](const boost::system::error_code &ec) { chasePrice(ec); });
  ioc_webSocket.run();
}

void ByBit::read_public_Socket() { wsPublic.read(wsBuffer); }

bool ByBit::socket_is_opened() { return wsPrivate.is_open() /*&& wsPublic.is_open()*/; }

void ByBit::write_private_Socket(const std::string &text) { wsPrivate.write(net::buffer(text)); }

void ByBit::write_public_Socket(const std::string &text) { wsPublic.write(net::buffer(text)); }

void ByBit::webSocket_close() {
  if (wsPrivate.is_open()) wsPrivate.close(websocket::close_code::normal);
  if (wsPublic.is_open()) wsPublic.close(websocket::close_code::normal);
}

std::string ByBit::get_socket_data() const noexcept { return beast::buffers_to_string(wsBuffer.data()); }

void ByBit::setCallback(const std::function<void(beast::error_code, size_t)> &new_callback) {
  callback = new_callback;
}

void ByBit::cancelOrders() {
  output << "Cancelling orders..." << std::endl;
  preparePrivateReq(cancelParams, cancelRequest);
  http::write(stream, cancelRequest);
  try {
    http::read(stream, buffer, cancelRes);
    //output << beast::buffers_to_string(cancelRes.body().data()) << std::endl;
    cancelRes.body().clear();
    output << "Orders cancelled (if there were any)" << std::endl;
  }
  catch (...) { output << "Couldn't read inside cancelOrders()" << std::endl; }
}

//Maybe remove assert and ifs to lower latency
void ByBit::placeBoth(std::pair<long double, long double> price) {
  auto [bidPrice, askPrice] = price;

  // setting up long parameters
  bidPrice -= delta;
  orderParams["request"][0] = longPosParams;
  orderParams["request"][0]["price"] = priceToStr(bidPrice);
  orderParams["request"][0]["takeProfit"] = priceToStr(bidPrice + bidPrice * takeProfit + delta);
  orderParams["request"][0]["stopLoss"] = priceToStr(bidPrice - bidPrice * stopLoss + delta);
  orderParams["request"][0]["orderLinkId"] = longLinkId;

  // setting up short parameters
  askPrice += delta;
  orderParams["request"][1] = shortPosParams;
  orderParams["request"][1]["price"] = priceToStr(askPrice);
  orderParams["request"][1]["takeProfit"] = priceToStr(askPrice - askPrice * takeProfit - 1.5*delta);
  orderParams["request"][1]["stopLoss"] = priceToStr(askPrice + askPrice * stopLoss - 1.5*delta);
  orderParams["request"][1]["orderLinkId"] = shortLinkId;

  preparePrivateReq(orderParams, orderRequest);
  http::write(stream, orderRequest);
  http::read(stream, buffer, orderRes);
  orderRes.body().clear();

  orderParams["request"].clear();
  output << "Both orders placed" << std::endl;
}

void ByBit::placeLong(long double bidPrice) {
  bidPrice -= delta;
  orderParams["request"][0] = longPosParams;
  orderParams["request"][0]["price"] = priceToStr(bidPrice);
  orderParams["request"][0]["takeProfit"] = priceToStr(bidPrice + bidPrice * takeProfit + delta);
  orderParams["request"][0]["stopLoss"] = priceToStr(bidPrice - bidPrice * stopLoss + delta);
  orderParams["request"][0]["orderLinkId"] = longLinkId;

  preparePrivateReq(orderParams, orderRequest);
  http::write(stream, orderRequest);
  try { http::read(stream, buffer, orderRes); }
  catch (std::exception &e) {
    output << "placeLong exception" << std::endl;
  }
  orderRes.body().clear();

  orderParams["request"].clear();
  //output << "Long order placed" << std::endl;
}

void ByBit::placeShort(long double askPrice) {
  askPrice += delta;
  orderParams["request"][0] = shortPosParams;
  orderParams["request"][0]["price"] = priceToStr(askPrice);
  orderParams["request"][0]["takeProfit"] = priceToStr(askPrice - askPrice * takeProfit - 1.5*delta);
  orderParams["request"][0]["stopLoss"] = priceToStr(askPrice + askPrice * stopLoss - 1.5*delta);
  orderParams["request"][0]["orderLinkId"] = shortLinkId;

  preparePrivateReq(orderParams, orderRequest);
  http::write(stream, orderRequest);
  try { http::read(stream, buffer, orderRes); }
  catch (std::exception &e) {
    output << "placeShort exception" << std::endl;
  }
  orderRes.body().clear();

  orderParams["request"].clear();
  //output << "Short order placed" << std::endl;
}

void ByBit::changeBoth(std::pair<long double, long double> price) {
  auto [bidPrice, askPrice] = price;

  // setting up long changes
  bidPrice -= delta;
  amendParams["request"][0]["symbol"] = bot.ticker;
  amendParams["request"][0]["price"] = priceToStr(bidPrice);
  amendParams["request"][0]["takeProfit"] = priceToStr(bidPrice + bidPrice * takeProfit + delta);
  amendParams["request"][0]["stopLoss"] = priceToStr(bidPrice - bidPrice * stopLoss + delta);
  amendParams["request"][0]["orderLinkId"] = longLinkId;

  // setting up short changes
  askPrice += delta;
  amendParams["request"][1]["symbol"] = bot.ticker;
  amendParams["request"][1]["price"] = priceToStr(askPrice);
  amendParams["request"][1]["takeProfit"] = priceToStr(askPrice - askPrice * takeProfit - 1.5*delta);
  amendParams["request"][1]["stopLoss"] = priceToStr(askPrice + askPrice * stopLoss - 1.5*delta);
  amendParams["request"][1]["orderLinkId"] = shortLinkId;

  preparePrivateReq(amendParams, amendRequest);
  http::write(stream, amendRequest);
  try { http::read(stream, buffer, changeRes); }
  catch (std::exception &e) {
    output << "changeBoth exception" << std::endl;
  }

  changeRes.body().clear();
  amendParams["request"].clear();
  //output << "Changed both orders" << std::endl;
}

void ByBit::changeLong(long double bidPrice) {
  bidPrice -= delta;
  amendParams["request"][0]["symbol"] = bot.ticker;
  amendParams["request"][0]["price"] = priceToStr(bidPrice);
  amendParams["request"][0]["takeProfit"] = priceToStr(bidPrice + bidPrice * takeProfit + delta);
  amendParams["request"][0]["stopLoss"] = priceToStr(bidPrice - bidPrice * stopLoss + delta);
  amendParams["request"][0]["orderLinkId"] = longLinkId;

  preparePrivateReq(amendParams, amendRequest);
  http::write(stream, amendRequest);
  try { http::read(stream, buffer, changeRes); }
  catch (std::exception &e) {
    output << "changeLong exception" << std::endl;
  }

  changeRes.body().clear();
  amendParams["request"].clear();
  //output << "Changed long order" << std::endl;
}

void ByBit::changeShort(long double askPrice) {
  askPrice += delta;
  amendParams["request"][0]["symbol"] = bot.ticker;
  amendParams["request"][0]["price"] = priceToStr(askPrice);
  amendParams["request"][0]["takeProfit"] = priceToStr(askPrice - askPrice * takeProfit - 1.5*delta);
  amendParams["request"][0]["stopLoss"] = priceToStr(askPrice + askPrice * stopLoss - 1.5*delta);
  amendParams["request"][0]["orderLinkId"] = shortLinkId;

  preparePrivateReq(amendParams, amendRequest);
  http::write(stream, amendRequest);
  try { http::read(stream, buffer, changeRes); }
  catch (std::exception &e) {
    output << "changeShort exception" << std::endl;
  }

  changeRes.body().clear();
  amendParams["request"].clear();
  //output << "Changed short order" << std::endl;
}

void ByBit::setLeverage() {
  const json leverage = {
    {"category", "linear"},
    {"symbol", bot.ticker},
    {"buyLeverage", bot.leverage},
    {"sellLeverage", bot.leverage}
  };
  auto req = getBasePrivateReq("/v5/position/set-leverage", http::verb::post);

  preparePrivateReq(leverage, req);
  http::write(stream, req);
  http::read(stream, buffer, res);

  json answer = json::parse(beast::buffers_to_string(res.body().data()));
  if (answer["retCode"] != 0 && answer["retMsg"] != "leverage not modified")
    output << req << std::endl << answer << std::endl;

  res.body().clear();
}

void ByBit::setHedgingMode(bool on) {
  json hedgingMode = {{"setHedgingMode", on ? "ON" : "OFF"}};
  auto req = getBasePrivateReq("/v5/account/set-hedging-mode", http::verb::post);
  preparePrivateReq(hedgingMode, req);
  http::write(stream, req);
  http::read(stream, buffer, res);
  res.body().clear();
}

//returns a pair of bidPrice and askPrice
std::pair<long double, long double> ByBit::getTickerPrice() {
  http::write(stream, priceRequest);
  try { http::read(stream, priceBuffer, priceRes); }
  catch (std::exception &e) {
    priceBuffer.clear();
  }
  auto response = json::parse(beast::buffers_to_string(priceRes.body().cdata()));
  priceRes.body().clear();
  return {stod(static_cast<std::string>(response["result"]["list"][0]["bid1Price"])),
          stod(static_cast<std::string>(response["result"]["list"][0]["ask1Price"]))};
}

void ByBit::getInstrumentInfo() {
  auto req = getBasePublicReq("/v5/market/instruments-info?category=linear&symbol=" + bot.ticker, http::verb::get);
  http::write(stream, req);
  http::read(stream, buffer, res);

  auto response = json::parse(beast::buffers_to_string(res.body().cdata()));
  res.body().clear();

  int8_t quantityScale = 0;
  bool flag = false;
  std::string qtyStep = response["result"]["list"][0]["lotSizeFilter"]["qtyStep"];
  // digits after dot in qty
  for (char c: qtyStep) {
    quantityScale += flag;
    if (c == '.') flag = true;
  }
  // Price and quantity format (digits after dot), minimum price step
  priceFmt = std::format("%1.{0}f", static_cast<std::string>(response["result"]["list"][0]["priceScale"]));
  qtyFmt = std::format("%1.{0}f", quantityScale);
  delta = stod(static_cast<std::string>(response["result"]["list"][0]["priceFilter"]["tickSize"]));
}

std::string ByBit::getUSDTBalance() {
  auto req = getBasePrivateReq("/v5/account/wallet-balance?accountType=UNIFIED&coin=USDT", http::verb::get);
  const json parameters = {{"accountType", "UNIFIED"}, {"coin", "USDT"}};
  preparePrivateReq(parameters, req);

  http::write(stream, req);
  http::read(stream, buffer, res);

  auto response = json::parse(beast::buffers_to_string(res.body().cdata()));
  res.body().clear();
  return static_cast<std::string>(response["result"]["list"][0]["coin"][0]["walletBalance"]);
}

// <long, short>, true if order is filled
std::pair<bool, bool> ByBit::ordersAreActive() {
  auto req = getBasePrivateReq("/v5/order/realtime?category=linear&symbol=" + bot.ticker, http::verb::get);
  const json parameters = {{"category", "linear"}, {"symbol", bot.ticker}};
  preparePrivateReq(parameters, req);

  http::write(stream, req);
  http::read(stream, buffer, res);

  auto response = json::parse(beast::buffers_to_string(res.body().cdata()));
  res.body().clear();

  std::pair<bool, bool> activeOrders{false, false};
  for (const auto &[index, object]: response["result"]["list"].items()) {
    if (object["side"] == "Buy" && object["symbol"] == bot.ticker)
      activeOrders.second = true;
    else if (object["side"] == "Sell" && object["symbol"] == bot.ticker)
      activeOrders.first = true;
  }
  return activeOrders;
}

void ByBit::debug(const http::request<http::string_body> &req, const http::response<http::dynamic_body> &res) {
  output << req << std::endl;

  try {
    auto response = json::parse(beast::buffers_to_string(res.body().cdata()));
    output << response.dump(2) << std::endl;
  }
  catch (...) { output << beast::buffers_to_string(res.body().cdata()) << std::endl; }
}

void ByBit::buffer_clear() noexcept { wsBuffer.clear(); }

void ByBit::consume_buffer(size_t messageSize) noexcept { wsBuffer.consume(messageSize); }

std::string ByBit::getTickerName() const noexcept { return bot.ticker; }

inline std::string ByBit::priceToStr(long double price) noexcept {
  return (boost::format(priceFmt) % price).str();
}

inline std::string ByBit::qtyToStr(long double qty) noexcept {
  return (boost::format(qtyFmt) % qty).str();
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
  wsPrivate.handshake(bot.webSocketPrivate, "/v5/private?max_active_time=10m");
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

http::request<http::string_body> ByBit::getBasePrivateReq(const std::string &endpoint, http::verb httpVerb) const {
  http::request<http::string_body> req{httpVerb, endpoint, 11};

  req.set(http::field::host, bot.host);
  req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
  req.set("X-BAPI-API-KEY", bot.api_key);
  req.set("X-BAPI-SIGN-TYPE", "2");
  req.set("X-BAPI-RECV-WINDOW", bot.recvWindow);
  req.set(http::field::content_type, "application/json");
  req.keep_alive(true);
  req.set(boost::beast::http::field::connection, "keep-alive");
  return req;
}

http::request<http::string_body> ByBit::getBasePublicReq(const std::string &endpoint, http::verb httpVerb) const {
  http::request<http::string_body> req{httpVerb, endpoint, 11};

  req.set(http::field::host, bot.host);
  req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
  req.set(http::field::content_type, "application/json");
  req.keep_alive(true);
  req.set(boost::beast::http::field::connection, "keep-alive");
  return req;
}

inline void ByBit::preparePrivateReq(const json &data, http::request<http::string_body> &req) const {
  std::string Timestamp = std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>
                                           (std::chrono::system_clock::now().time_since_epoch()).count());
  req.set("X-BAPI-TIMESTAMP", Timestamp);
  if (req.method() == http::verb::post) {
    req.set("X-BAPI-SIGN", Encryption::GeneratePostSignature(data, bot, Timestamp));
    req.body() = data.dump();
  }
  else {
    req.set("X-BAPI-SIGN", Encryption::GenerateGetSignature(data, bot, Timestamp));
  }
  req.prepare_payload();
}

void ByBit::beginAsyncPings(const boost::system::error_code &ec) {
  if (ec) {
    output << "(timer) " << ec.message() << std::endl;
    throw std::exception();
  }
  wsPrivate.async_ping(R"({"op":"ping"})",
                       [this](const beast::error_code &ec) {
                         if (ec) {
                           output << "(ping) " << ec.message();
                           throw std::exception();
                         }
                         //output << "Pinged [" << currTimeMS << "]" << std::endl;
                         this->m_timer.expires_after(net::chrono::seconds(10));
                         this->m_timer
                           .async_wait([this](const boost::system::error_code &ec) { this->beginAsyncPings(ec); }
                           );
                       }
  );
}

void ByBit::chasePrice(const boost::system::error_code &ec) {
  if (ec) {
    output << "(chasePrice) " << ec.message() << std::endl;
    throw std::exception();
  }
  // Change current price
  //if (longNotFilled && shortNotFilled)
  changeBoth(getTickerPrice());
//  else if (longNotFilled)
//    changeLong(getTickerPrice().first);
//  else if (shortNotFilled)
//    changeShort(getTickerPrice().second);

  chaseTimer.expires_after(net::chrono::milliseconds(bot.updatePriceInterval));
  chaseTimer.async_wait(
    [this](const boost::system::error_code &ec) { this->chasePrice(ec); }
  );
}

void ByBit::initOrderReq() {
  orderRequest = getBasePrivateReq("/v5/order/create-batch", http::verb::post);
}

void ByBit::initPriceReq() {
  priceRequest = getBasePublicReq("/v5/market/tickers?category=linear&symbol=" + bot.ticker, http::verb::get);
}

void ByBit::initCancelReq() {
  cancelParams["symbol"] = bot.ticker;
  cancelRequest = getBasePrivateReq("/v5/order/cancel-all", http::verb::post);
}

void ByBit::initAmendRequest() {
  amendRequest = getBasePrivateReq("/v5/order/amend-batch", http::verb::post);
}
