#include "../../include/core/Application.hpp"

#include "../../include/protocol/RdtReceiver.hpp"
#include "../../include/protocol/RdtSender.hpp"

#include <cerrno>
#include <csignal>
#include <iostream>
#include <poll.h>
#include <system_error>
#include <unistd.h>

// Initialize the execution flag
volatile sig_atomic_t Application::is_running = 1;

// Signal handler
void Application::signal_handler(int) { is_running = 0; }

Application::Application(const Config &config)
    : config(config), timer_manager(config.get_timeout()) {

  // Set up signal handlers
  std::signal(SIGINT, Application::signal_handler);
  std::signal(SIGTERM, Application::signal_handler);

  // Initialize network endpoints
  if (config.get_mode() == Config::Mode::SERVER) {
    socket.bind(config.get_port(), config.get_target_ip());
    endpoint = std::make_unique<RdtReceiver>(socket, timer_manager, config);
  } else {
    // Client
    socket.set_target(config.get_resolved_address(),
                      config.get_resolved_address_length());
    endpoint = std::make_unique<RdtSender>(socket, timer_manager, config);
  }
};

Application::~Application() {
  // Memory is freed automatically via std::unique_ptr
  // UdpSocket's destructor will automatically close the socket
};

void Application::run() {
  // Wait for up to 2 file descriptors
  // fds[0] = UDP Socket
  // fds[1] = Standard Input / File in case of Sender
  struct pollfd fds[2];
  int num_fds = 1; // Initializing the number of fds at 1 and conditionally
                   // incrementing it when we need another file descriptor.

  // Socket is monitored always
  fds[0].fd = socket.get_fd();
  fds[0].events = POLLIN; // Data ready to be read event

  // If the endpoint has local file descriptor to monitor
  // e.g., Sender reading a file.
  int io_fd = endpoint->get_io_fd();
  if (io_fd != -1) {
    fds[1].fd = io_fd;
    fds[1].events = POLLIN; // Data ready to be read event
    num_fds = 2;            // Increment the number of file descriptors
  }

  // Main event loop
  while (is_running && !endpoint->is_transfer_complete()) {
    // Ask the timer manager how long until next retransmission timeout or
    // global timeout
    int poll_timeout_ms = timer_manager.get_next_timeout_ms();

    // Block until an event occurs or the timeout expires
    int ret = ::poll(fds, num_fds, poll_timeout_ms);

    if (ret < 0) {
      if (errno == EINTR) {
        // Interrupted by signal
        // is_running was just set to 0, so the loop will break on the next
        // check
        continue;
      }

      throw std::system_error(errno, std::system_category(), "poll() failed");
    }

    if (ret == 0) {
      // No file descriptors are ready, meaning timeout occured
      endpoint->handle_timeout();
      continue;
    }

    // HANDLE NETWORK EVENTS
    // revents = types of events that actually occured
    // revents & POLLIN = there is data to read event
    if (fds[0].revents & POLLIN) {
      endpoint->handle_network_event();
    }

    // HANDLE I/O EVENTS
    if (num_fds == 2 && (fds[1].revents & POLLIN)) {
      endpoint->handle_io_event();
    }
  }

  // Teardown
  if (!is_running) {
    std::cerr << "Application interrupted by signal. Tearing down...\n";
    // TODO: the endpoint could send a final RST or FIN packet here
    endpoint->handle_interrupt();
  }
}
