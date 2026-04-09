#include "../include/cli_parser.h"

#include <exception>

int main(int argc, char *const *argv) {
  try {

    ScannerConfig config = CliParser::parse(argc, argv);

    if (config.help_requested) {
      return 0;
    }

  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << "\n";
    return 1;
  }

  return 0;
}
