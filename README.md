# ByBit-bot

Bybit crypto exchange trading bot. Made for my educational purposes only.

# What it does

The bot will place short and long trading orders on a specified trading pair. Once either order is executed, it will cancel the other one and place 2 orders again. If neither order is executed after a specified time, the bot will replace both orders closer to the new price.

# Parameters

In parameters.json the following parameters can be specified:

{

  "api_key" : "xxxxxxxxxxxxxxxxxx", //public API key
  
  "secret_key" : "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", //private API key
  
  "ticker" : "ETHUSDT", //trading pair
  
  "host" : "api-demo.bybit.com", //demo or real network
  
  "webSocketPrivate" : "stream-demo.bybit.com", //demo or real network
  
  "webSocketPublic" : "stream.bybit.com", //demo or real network
  
  "recvWindow" : "3000", //ms, don't execute API request after a certain time
  
  "leverage" : "5", //leverage for short and long positions
  
  "port" : "443", //https port
  
  "takeProfit": 0.082, // %

  "stopLoss": 0.105, // %
  
  "approxQuantityUSD" : 1000, // money used for borth orders combined at the launch
  
  "updatePriceInterval": 1000, //ms, replace both orders if neither are executed
  
  "syncOrdersInterval": 3600 //seconds, update trading quanity to match approxQuantityUSD
  
}
