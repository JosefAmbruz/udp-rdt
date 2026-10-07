#include "core/Config.hpp"

#include <climits>
#include <cstring>
#include <iostream>
#include <stdexcept>

Config::Config(int argc, char *const *argv) {
  bool has_server_f = false;
  bool has_client_f = false;
  bool has_port = false;

  opterr = 0;
  int opt;

  static struct option long_options[] = {{"help", no_argument, 0, 'h'},
                                         {0, 0, 0, 0}};

  int option_index = 0;
  while ((opt = getopt_long(argc, argv, "hsca:p:i:o:w:", long_options,
                            &option_index)) != -1) {
    switch (opt) {
    case 'h':
      std::cout << "Usage Server: ./udp-rdt -s -p PORT [-a ADDRESS] [-o "
                   "OUTPUT] [-w TIMEOUT]\n";
      std::cout << "Usage Client: ./udp-rdt -c -a HOST -p PORT [-i INPUT] [-w "
                   "TIMEOUT]\n";
      exit(0);
    case 's': // Server flag
      has_server_f = true;
      break;
    case 'c': // Client flag
      has_client_f = true;
      break;
    case 'a': // Used as HOST for client and ADDRESS for server
      target_ip = optarg;
      break;
    case 'p': // Port
      try {
        int parsed_port = std::stoi(optarg);
        if (parsed_port <= 0 || parsed_port > USHRT_MAX) {
          throw std::out_of_range("");
        }
        port = static_cast<uint16_t>(parsed_port);
        has_port = true;
      } catch (const std::exception &e) {
        throw std::invalid_argument(
            std::string("Invalid port specified with -p: must be a valid "
                        "number between 1 and 65535."));
      }
      break;
    case 'i': // Input file
      input_file = optarg;
      break;
    case 'o': // Output file
      output_file = optarg;
      break;
    case 'w': // Wait time in seconds (default: 1s)
      try {
        int parsed_timeout = std::stoi(optarg);
        if (parsed_timeout <= 0) {
          throw std::out_of_range("");
        }
        timeout = std::chrono::seconds(parsed_timeout);
      } catch (const std::exception &) {
        throw std::invalid_argument(
            "Invalid timeout specified with -w, must be a positive number.");
      }
      break;
    case '?':
      throw std::invalid_argument(
          std::string("Unknown option or missing argument for -") +
          static_cast<char>(optopt));
    default:
      throw std::invalid_argument("Error parsing command line arguments.");
    }
  }

  // Validation rules
  if (has_server_f && has_client_f) {
    throw std::invalid_argument(
        "Mutually exclusive flags: Cannot specify both -s and -c");
  }
  if (!has_server_f && !has_client_f) {
    throw std::invalid_argument(
        "Missing mode: Must specify either -s or -c flag");
  }
  if (!has_port) {
    throw std::invalid_argument("Missing port: You must specify port with -p");
  }

  mode = has_server_f ? Mode::SERVER : Mode::CLIENT;

  // The -a flag is mandatory for the client but optional for the server
  if (mode == Mode::CLIENT) {
    if (target_ip.empty()) {
      throw std::invalid_argument(
          "Client mode requires a destination host specified with -a");
    }

    resolve_address();
  } else if (mode == Mode::SERVER && !target_ip.empty()) {
    resolve_address();
  }

  // NOTE: If target_ip is empty, UdpSocket will bind to INADDR_ANY
  // automatically.
}

void Config::resolve_address() {
  struct addrinfo hints{};
  struct addrinfo *result = nullptr;

  std::memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;    // Allow IPv4 or IPv6
  hints.ai_socktype = SOCK_DGRAM; // We are using UDP

  std::string port_str = std::to_string(port);
  int status =
      getaddrinfo(target_ip.c_str(), port_str.c_str(), &hints, &result);
  if (status != 0) {
    throw std::invalid_argument(
        std::string("Failed to resolve IP/Hostname: ") +
        gai_strerror(
            status)); // Converts error code returned from getaddrinfo to string
  }

  // Copy the first successfully resolved address to sockaddr_storage field.
  if (result != nullptr) {
    std::memcpy(&resolved_addr, result->ai_addr, result->ai_addrlen);
    resolved_addr_len = result->ai_addrlen;
    freeaddrinfo(result);
  } else {
    throw std::invalid_argument("Failed to resolve address: No results found.");
  }
}

// Getters
Config::Mode Config::get_mode() const { return mode; }
uint16_t Config::get_port() const { return port; }
const std::string &Config::get_target_ip() const { return target_ip; }
const std::optional<std::string> &Config::get_input_file() const {
  return input_file;
}
const std::optional<std::string> &Config::get_output_file() const {
  return output_file;
}
std::chrono::seconds Config::get_timeout() const { return timeout; }
const struct sockaddr_storage &Config::get_resolved_address() const {
  return resolved_addr;
}
socklen_t Config::get_resolved_address_length() const {
  return resolved_addr_len;
}
