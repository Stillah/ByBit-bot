#include "Strategy.h"

[[noreturn]] int main() {
  const std::string logFile = "logs/Bybit log " + currDateAndTime();
  std::ofstream output(logFile, std::ios::app);

  std::cout << "Bot is working, find logs at " << logFile << std::endl;
  output << "----------------------------"
            "     Program started at " << currDateAndTime() << "     "
            "----------------------------\n" << std::endl;
  output << "     Parameters:     " << std::endl;
  int32_t takeProfits = 0, stopLoses = 0;
  long double fees = 0, generatedVolume = 0;

  try {
    std::ifstream file("parameters.json", std::ios::in);
    // Removing parameters that don't need to be displayed
    json params = json::parse(file);
    params.erase("port");
    params.erase("recvWindow");
    params.erase("secret_key");
    params.erase("webSocketPublic");

    output << params.dump(2) << std::endl;
    output <<
           "\n---------------------------------------------------------------------------------------------------------"
           "\n\n" << std::endl;
    output.close();
  }
  catch (std::exception &e) {
    output << e.what() << std::endl;
    assert(false);
  }

  while (true) {
    Strategy("parameters.json", logFile, takeProfits, stopLoses, fees, generatedVolume);
    output << "Restarting the bot after 3 seconds" << std::endl;
    sleep(3);
  }
}
