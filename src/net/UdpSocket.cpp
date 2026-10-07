#include "net/UdpSocket.hpp"

#include <cerrno>
#include <cstdint>
#include <cstring>
#include <netdb.h>
#include <netinet/in.h>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <system_error>
#include <unistd.h>

UdpSocket::UdpSocket() : fd(-1) {}

UdpSocket::~UdpSocket() {
  if (fd >= 0) {
    ::close(fd);
  }
}

void UdpSocket::create_socket(int domain) {
  if (fd >= 0) {
    // Socket already created.
    return;
  }

  fd = ::socket(domain, SOCK_DGRAM,
                0); // 0 means protocol is chosen automatically

  if (fd < 0) {
    throw std::system_error(errno, std::system_category(),
                            "Failed to create UDP socket.");
  }

  // If it's a IPv6 address, accept both IPv4 and IPv6 traffic
  if (domain == AF_INET6) {
    int opt = 0; // false
    ::setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, &opt, sizeof(opt));
  }
}

void UdpSocket::bind(uint16_t port, const std::string &local_address) {
  struct addrinfo hints{}, *result = nullptr;

  std::memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC; // Both IPv4 and IPv6
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_flags = AI_PASSIVE;

  std::string port_str = std::to_string(port);
  const char *node = local_address.empty() ? nullptr : local_address.c_str();

  int status = getaddrinfo(node, port_str.c_str(), &hints, &result);
  if (status != 0) {
    throw std::invalid_argument(
        std::string("Bind failed to resolve address: ") + gai_strerror(status));
  }

  // Try to create and bind the socket using the first valid result
  struct addrinfo *rp;
  for (rp = result; rp != nullptr; rp = rp->ai_next) {
    try {
      create_socket(rp->ai_family);
      if (::bind(fd, rp->ai_addr, rp->ai_addrlen) == 0) {
        break; // Success
      }
      // If bind fails, close the FD and try the next address in the list
      ::close(fd);
      fd = -1;
    } catch (...) {
      continue;
    }
  }

  freeaddrinfo(result);

  if (rp == nullptr) {
    throw std::system_error(errno, std::system_category(),
                            "Failed to bind UDP socket to any address");
  }
}

void UdpSocket::set_target(const struct sockaddr_storage &addr,
                           socklen_t addr_len) {

  // Determine if the provided address is IPv4 or IPv6 and create the socket
  create_socket(addr.ss_family);

  std::memcpy(&target_addr, &addr, addr_len);
  target_addr_len = addr_len;
  target_set = true;
}

ssize_t UdpSocket::send(const std::vector<uint8_t> &data) {
  if (!target_set) {
    throw std::logic_error("Cannot send: target address is not set.");
  }
  return send_to(data, target_addr, target_addr_len);
}

ssize_t UdpSocket::send_to(const std::vector<uint8_t> &data,
                           const struct sockaddr_storage &dest_addr,
                           socklen_t addr_len) {
  if (fd < 0) {
    throw std::logic_error("Cannot send: socket is not initialized.");
  }

  ssize_t sent =
      ::sendto(fd, data.data(), data.size(), 0,
               reinterpret_cast<const struct sockaddr *>(&dest_addr), addr_len);

  if (sent < 0) {
    throw std::system_error(errno, std::system_category(), "sendto() failed");
  }
  return sent;
}

ssize_t UdpSocket::receive(std::vector<uint8_t> &buffer,
                           struct sockaddr_storage &src_addr,
                           socklen_t &src_addr_len) {
  if (fd < 0) {
    throw std::logic_error("Cannot receive: socket is not initialized.");
  }

  src_addr_len = sizeof(src_addr);

  // Resize the buffer to the maximum possible size we expect (1200)
  buffer.resize(1200);

  ssize_t received =
      ::recvfrom(fd, buffer.data(), buffer.size(), 0,
                 reinterpret_cast<struct sockaddr *>(&src_addr), &src_addr_len);

  if (received < 0) {
    throw std::system_error(errno, std::system_category(), "recvfrom() failed");
  }

  // Shrink the vector down to the actual number of bytes received
  buffer.resize(received);

  return received;
}

int UdpSocket::get_fd() const { return fd; }
