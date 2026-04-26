#include "../include/core/Application.hpp"
#include "../include/core/Config.hpp"

#include <exception>
#include <ios>
#include <iostream>
#include <stdexcept>
#include <sysexits.h>
#include <system_error>

int main(int argc, char *const *argv) {
  try {
    Config config(argc, argv);

    Application app(config);

    app.run();

    return EX_OK;

  } catch (const std::invalid_argument &e) {
    // Thrown by Config if arguments are incorrect
    std::cerr << "Usage Error: " << e.what() << "\n";
    return EX_USAGE; // 64
  } catch (const std::ios_base::failure &e) {
    // Thrown by RdtSender or Receiver if r/w to file fails
    std::cerr << "I/O Error: " << e.what() << "\n";
    return EX_IOERR; // 74
  } catch (const std::system_error &e) {
    // Thrown by UdpSocket or poll() if OS-level operations fail
    std::cerr << "System/Network Error: " << e.what() << "\n";
    return EX_OSERR; // 71
  } catch (const std::exception &e) {
    // Generic fallback
    std::cerr << "Internal Error: " << e.what() << "\n";
    return EX_SOFTWARE; // 70
  } catch (...) {
    // Catch all
    std::cerr << "An unknown error occured.\n";
    return EX_SOFTWARE;
  }
}
