#pragma once

#include "../net/UdpSocket.hpp"
#include "../protocol/RdtEndpoint.hpp"
#include "../protocol/TimerManager.hpp"
#include "Config.hpp"

#include <csignal> // For SIGINT, SIGTERM
#include <memory>

class Application {
public:
  /**
   * @brief The main orchestrator of the application.
   * @param config Validated command line configuration.
   */
  explicit Application(const Config &config); // explicit is best practice for
                                              // single param constructors

  /**
   * @brief Destructor ensures clean teardown of resources.
   */
  ~Application();

  /**
   * @brief Starts the main loop. Blocks until the loop completes or signal is
   * caught.
   */
  void run();

private:
  const Config &config;
  UdpSocket socket;
  TimerManager timer_manager;

  // Points to either RdtSender or RdtReceiver
  std::unique_ptr<RdtEndpoint> endpoint;

  // Signal handling variables
  static volatile sig_atomic_t is_running;
  static void signal_handler(int signum);
};
