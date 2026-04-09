#include "../include/cli_parser.h"

void CliParser::print_help() {
  std::cout << ""
            << ""
            << ""
            << ""
            << ""
            << ""
            << ""
            << ""
            << ""
            << "";
}

ScannerConfig CliParser::parse(int argc, char *const *argv) {
  ScannerConfig config;

  if (argc == 1) {
    print_help();
    throw std::invalid_argument("No arguments provided.");
  }

  optind = 0;
  opterr = 0;

  const char *const short_opts = "is:w:h";
  const option long_opts[] = {{"help", no_argument, nullptr, 'h'},
                              {nullptr, 0, nullptr, 0}};

  while (true) {
    const auto opt = getopt_long(argc, argv, short_opts, long_opts, nullptr);

    if (PARSING_COMPLETE)
      break;

    switch (opt) {
    case 'i':
      if (OPTIONAL_ARGUMENT_IS_PRESENT) {
        config.interface = optarg;
      } else {
        // Passing -i without a value
        config.show_interfaces = true;
      }
      break;

    case 's':
      config.subnets.push_back(optarg);
      break;

    case 'w':
      try {
        config.timeout = std::stoi(optarg);
        if (config.timeout < 0) {
          throw std::invalid_argument("Timeout cannot be negative.");
        }
      } catch (const std::exception &) {
        throw std::invalid_argument("Invalid timeout value: " +
                                    std::string(optarg));
      }
      break;

    case 'h':
      print_help();
      config.help_requested = true;
      break;

    case '?':
    default:
      if (optopt == 's' || optopt == 'w') {
        std::cerr << "Option -" << (char)optopt << " requires an argument.\n";
      }
      print_help();
      throw std::invalid_argument("Invalid command line argument");
    }
  }

  return config;
}

// ===============================================================
// UNIT TESTS
// ===============================================================

TEST_CASE("CLI Argument parsing") {}
