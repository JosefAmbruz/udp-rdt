#pragma once

#include <chrono>
#include <cstdint>
#include <getopt.h>
#include <netinet/in.h>
#include <optional>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>

class Config {
public:
  enum class Mode { SERVER, CLIENT };

  /**
   * @brief Constructs the Config by passing argc and argv
   */
  Config(int argc, char *const *argv);

  // Getters for the parsed and validated configuration
  Mode get_mode() const;
  uint16_t get_port() const;
  const std::string &get_target_ip() const;

  // Input/Output files might be ommited
  const std::optional<std::string> &get_input_file() const;
  const std::optional<std::string> &get_output_file() const;

  std::chrono::seconds get_timeout() const;

  // Resolved network address for UdpSocket
  const struct sockaddr_storage &get_resolved_address() const;
  socklen_t get_resolved_address_length() const;

private:
  Mode mode;
  uint16_t port = 0;
  std::string target_ip;
  std::optional<std::string> input_file;
  std::optional<std::string> output_file;
  std::chrono::seconds timeout{1}; // Default timeout is 1 second

  // Network structs
  struct sockaddr_storage resolved_addr{};
  socklen_t resolved_addr_len = 0;

  /**
   * @brief Resolves the target ip and port to the resolved_addr structure.
   */
  void resolve_address();
};
