#include "ByBit.h"
#include "Encryption.hpp"
#include <boost/format.hpp>


ByBit::ByBit(const std::string &file, std::string &longLinkIdRef, std::string &shortLinkIdRef)
  : longLinkId(longLinkIdRef),
    shortLinkId(shortLinkIdRef),
    m_timer(ioc_webSocket),
    chaseTimer(ioc_webSocket) {
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
  longPosParams["qty"] = shortPosParams["qty"] = qtyToStr(bot.quantity / 2);
  takeProfit = bot.takeProfit / 100;
  stopLoss = bot.stopLoss / 100;
  std::cerr << "Bot finished construction\n";
}

ByBit::~ByBit() {
  std::cerr << "Cancelling orders...\n";
  cancelOrders();
  webSocket_close();
  std::cerr << "Orders cancelled\n";
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
  if (wsPrivate.is_open()) wsPrivate.close(websocket::close_code::none);
  if (wsPublic.is_open()) wsPublic.close(websocket::close_code::none);
}

std::string ByBit::get_socket_data() const noexcept { return beast::buffers_to_string(wsBuffer.data()); }

void ByBit::setCallback(const std::function<void(beast::error_code, size_t)> &new_callback) {
  callback = new_callback;
}

void ByBit::cancelOrders() {
  preparePrivateReq(cancelParams, cancelRequest);
  http::write(stream, cancelRequest);
  http::read(stream, buffer, cancelRes);
  std::cerr << beast::buffers_to_string(cancelRes.body().data());
  cancelRes.body().clear();
}

//Maybe remove assert and ifs to lower latency
void ByBit::placeBoth(std::pair<double, double> price) {
  auto [bidPrice, askPrice] = price;

  // setting up long parameters
  orderParams["request"][0] = longPosParams;
  orderParams["request"][0]["price"] = priceToStr(bidPrice - delta);
  orderParams["request"][0]["takeProfit"] = priceToStr(bidPrice + bidPrice * takeProfit);
  orderParams["request"][0]["stopLoss"] = priceToStr(bidPrice - bidPrice * stopLoss);
  orderParams["request"][0]["orderLinkId"] = longLinkId;

  // setting up short parameters
  orderParams["request"][1] = shortPosParams;
  orderParams["request"][1]["price"] = priceToStr(askPrice + delta);
  orderParams["request"][1]["takeProfit"] = priceToStr(askPrice - askPrice * takeProfit);
  orderParams["request"][1]["stopLoss"] = priceToStr(askPrice + askPrice * stopLoss);
  orderParams["request"][1]["orderLinkId"] = shortLinkId;

  preparePrivateReq(orderParams, orderRequest);
  http::write(stream, orderRequest);
  http::read(stream, buffer, orderRes);
  orderRes.body().clear();

  orderParams["request"].clear();
  std::cerr << "Both orders placed\n";
}

void ByBit::placeLong(double bidPrice) {
  orderParams["request"][0] = longPosParams;
  orderParams["request"][0]["price"] = priceToStr(bidPrice - delta);
  orderParams["request"][0]["takeProfit"] = priceToStr(bidPrice + bidPrice * takeProfit);
  orderParams["request"][0]["stopLoss"] = priceToStr(bidPrice - bidPrice * stopLoss);
  orderParams["request"][0]["orderLinkId"] = longLinkId;

  preparePrivateReq(orderParams, orderRequest);
  http::write(stream, orderRequest);
  try { http::read(stream, buffer, orderRes); }
  catch (std::exception &e) {
    std::cerr << "placeLong exception\n";
  }
  orderRes.body().clear();

  orderParams["request"].clear();
  std::cerr << "Long order placed\n";
}

void ByBit::placeShort(double askPrice) {
  orderParams["request"][0] = shortPosParams;
  orderParams["request"][0]["price"] = priceToStr(askPrice + delta);
  orderParams["request"][0]["takeProfit"] = priceToStr(askPrice - askPrice * takeProfit);
  orderParams["request"][0]["stopLoss"] = priceToStr(askPrice + askPrice * stopLoss);
  orderParams["request"][0]["orderLinkId"] = shortLinkId;

  preparePrivateReq(orderParams, orderRequest);
  http::write(stream, orderRequest);
  try { http::read(stream, buffer, orderRes); }
  catch (std::exception &e) {
    std::cerr << "placeShort exception\n";
  }
  orderRes.body().clear();

  orderParams["request"].clear();
  std::cerr << "Short order placed\n";
}

void ByBit::changeBoth(std::pair<double, double> price) {
  auto [bidPrice, askPrice] = price;

  // setting up long changes
  amendParams["request"][0]["symbol"] = bot.ticker;
  amendParams["request"][0]["price"] = priceToStr(bidPrice - delta);
  amendParams["request"][0]["takeProfit"] = priceToStr(bidPrice + bidPrice * takeProfit);
  amendParams["request"][0]["stopLoss"] = priceToStr(bidPrice - bidPrice * stopLoss);
  amendParams["request"][0]["orderLinkId"] = longLinkId;

  // setting up short changes
  amendParams["request"][1]["symbol"] = bot.ticker;
  amendParams["request"][1]["price"] = priceToStr(askPrice);
  amendParams["request"][1]["takeProfit"] = priceToStr(askPrice - askPrice * takeProfit);
  amendParams["request"][1]["stopLoss"] = priceToStr(askPrice + askPrice * stopLoss);
  amendParams["request"][1]["orderLinkId"] = shortLinkId;

  preparePrivateReq(amendParams, amendRequest);
  http::write(stream, amendRequest);
  try { http::read(stream, buffer, changeRes); }
  catch (std::exception &e) {
    std::cerr << "changeBoth exception\n";
  }

  changeRes.body().clear();
  amendParams["request"].clear();
  //std::cerr << "Changed both orders\n";
}

void ByBit::changeLong(double bidPrice) {
  amendParams["request"][0]["symbol"] = bot.ticker;
  amendParams["request"][0]["price"] = priceToStr(bidPrice - delta);
  amendParams["request"][0]["takeProfit"] = priceToStr(bidPrice + bidPrice * takeProfit);
  amendParams["request"][0]["stopLoss"] = priceToStr(bidPrice - bidPrice * stopLoss);
  amendParams["request"][0]["orderLinkId"] = longLinkId;

  preparePrivateReq(amendParams, amendRequest);
  http::write(stream, amendRequest);
  try { http::read(stream, buffer, changeRes); }
  catch (std::exception &e) {
    std::cerr << "changeLong exception\n";
  }

  changeRes.body().clear();
  amendParams["request"].clear();
  //std::cerr << "Changed long order\n";
}

void ByBit::changeShort(double askPrice) {
  amendParams["request"][0]["symbol"] = bot.ticker;
  amendParams["request"][0]["price"] = priceToStr(askPrice);
  amendParams["request"][0]["takeProfit"] = priceToStr(askPrice - askPrice * takeProfit);
  amendParams["request"][0]["stopLoss"] = priceToStr(askPrice + askPrice * stopLoss);
  amendParams["request"][0]["orderLinkId"] = shortLinkId;

  preparePrivateReq(amendParams, amendRequest);
  http::write(stream, amendRequest);
  try { http::read(stream, buffer, changeRes); }
  catch (std::exception &e) {
    std::cerr << "changeShort exception\n";
  }

  changeRes.body().clear();
  amendParams["request"].clear();
  //std::cerr << "Changed short order\n";
}

void ByBit::setLeverage() {
  json leverage = {
    {"category", "linear"},
    {"symbol", bot.ticker},
    {"buyLeverage", bot.leverage},
    {"sellLeverage", bot.leverage}
  };
  auto req = getBasePostReq("/v5/position/set-leverage");

  preparePrivateReq(leverage, req);
  http::write(stream, req);
  http::read(stream, buffer, res);

  json answer = json::parse(beast::buffers_to_string(res.body().data()));
  if (answer["retCode"] != 0 && answer["retMsg"] != "leverage not modified")
    std::cerr << req << "\n" << answer << "\n";

  res.body().clear();
}

void ByBit::setHedgingMode(bool on) {
  json hedgingMode = {{"setHedgingMode", on ? "ON" : "OFF"}};
  auto req = getBasePostReq("/v5/account/set-hedging-mode");
  preparePrivateReq(hedgingMode, req);
  http::write(stream, req);
  http::read(stream, buffer, res);
  res.body().clear();
}

//returns a pair of bidPrice and askPrice
std::pair<double, double> ByBit::getTickerPrice() {
  http::write(stream, priceRequest);
  try { http::read(stream, buffer, res); }
  catch (std::exception &e) {
    buffer_clear();
  }
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
  auto req = getBaseGetReq("/v5/account/wallet-balance?accountType=UNIFIED&coin=USDT");
  req.set("X-BAPI-API-KEY", bot.api_key);
  req.set("X-BAPI-SIGN-TYPE", "2");
  req.set("X-BAPI-RECV-WINDOW", bot.recvWindow);
  preparePrivateReq({}, req);
  debug(req);
  http::write(stream, req);
  http::read(stream, buffer, res);

  auto response = json::parse(beast::buffers_to_string(res.body().cdata()));
  res.body().clear();
  return static_cast<std::string>(response["result"]["list"][0]["coin"][0]["usdValue"]);
}

void ByBit::debug(const http::request<http::string_body> &req) {
  std::cerr << req << "\n";

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

inline std::string ByBit::qtyToStr(double qty) noexcept {
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
  req.keep_alive(true);
  req.set(boost::beast::http::field::connection, "keep-alive");
  return req;
}

[[nodiscard]] http::request<http::string_body> ByBit::getBaseGetReq(const std::string &endpoint) const {
  http::request<http::string_body> req{http::verb::get, endpoint, 11};

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
  req.set("X-BAPI-SIGN", Encryption::GeneratePostSignature(data, bot, Timestamp));
  if (!data.empty()) req.body() = data.dump();
  req.prepare_payload();
}

void ByBit::beginAsyncPings(const boost::system::error_code &ec) {
  if (ec) {
    std::cerr << "(timer) " << ec.message() << "\n";
    return;
  }
  wsPrivate.async_ping(R"({"op":"ping"})",
                       [this](const beast::error_code &ec) {
                         if (ec) {
                           std::cerr << "(ping) " << ec.message();
                           return;
                         }
                         //std::cerr << "Pinged [" << currTimeMS << "]\n";
                         this->m_timer.expires_after(net::chrono::seconds(20));
                         this->m_timer
                           .async_wait([this](const boost::system::error_code &ec) { this->beginAsyncPings(ec); }
                           );
                       }
  );
}

void ByBit::chasePrice(const boost::system::error_code &ec) {
  if (ec) {
    std::cerr << "(chasePrice) " << ec.message() << "\n";
    return;
  }
  // Change current price
  if (longNotFilled && shortNotFilled)
    changeBoth(getTickerPrice());
  else if (longNotFilled)
    changeLong(getTickerPrice().first);
  else if (shortNotFilled)
    changeShort(getTickerPrice().second);

  chaseTimer.expires_after(net::chrono::milliseconds(bot.updatePriceInterval));
  chaseTimer.async_wait(
    [this](const boost::system::error_code &ec) { this->chasePrice(ec); }
  );
}

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
  amendRequest = getBasePostReq("/v5/order/amend-batch");
}
