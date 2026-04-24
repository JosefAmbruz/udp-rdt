#pragma once

#include <cstdint>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <vector>

/**
 * @brief A wrapper for POSIX UDP socket handling both IPv4 and IPv6
 */
class UdpSocket {
public:
  UdpSocket();

  // Disable copying because this object owns a raw file descriptor
  // This is safety measure that prevents two object existing, both thinking
  // they exclusively own the same underlying OS socket. If this behavior was
  // allowed, when the second object would be destroyed, it would close already
  // closed file descriptor which is undefined behavior potentially closing
  // completely unrelated file, because OS reuses file descriptors.
  UdpSocket(const UdpSocket &) = delete;
  UdpSocket &operator=(const UdpSocket &) = delete;

  ~UdpSocket();

  /**
   * @brief SERVER MODE: Creates the socket and binds it to a port.
   * @param port The UDP port to listen to
   * @param local_address Optional local address to bind to.
   */
  void bind(uint16_t port, const std::string &local_address = "");

  /**
   * @brief CLIENT MODE: Creates the socket and sets the destination address
   * @param addr The resolved address
   * @param addr_len The lenght of the address
   */
  void set_target(const struct sockaddr_storage &addr, socklen_t addr_len);

  /**
   * @brief Sends data to the configured target address
   * @param data The raw bytes to send.
   * @return The number of bytes sent.
   */
  ssize_t send(const std::vector<uint8_t> &data);

  /**
   * @brief Sends data to a specific destination (used by server to reply to
   * clients).
   */
  ssize_t send_to(const std::vector<uint8_t> &data,
                  const struct sockaddr_storage &dest_addr, socklen_t addr_len);

  /**
   *
   */
  ssize_t receive(std::vector<uint8_t> &buffer,
                  struct sockaddr_storage &src_addr, socklen_t &src_addr_len);

  /**
   * @brief returns a raw file descriptor for use in poll()
   */
  int get_fd() const;

private:
  int fd;

  struct sockaddr_storage target_addr{};
  socklen_t target_addr_len = 0;
  bool target_set = false;

  // Helper wrapper to call OS socket() safely
  void create_socket(int domain);
};
