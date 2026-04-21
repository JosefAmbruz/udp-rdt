#include "../include/cli_parser.h"
#include "../include/doctest.h"

#include <getopt.h>
#include <iostream>
#include <stdexcept>
#include <string>

void CliParser::print_help() {
  std::cout
      << "Usage (Server):\n"
      << "  ./ipk-rdt -s -p PORT [-a ADDRESS] [-o OUTPUT] [-w TIMEOUT] [-h | "
         "--help]\n"
      << "\n"
      << "Usage (Client):\n"
      << "  ./ipk-rdt -c -a HOST -p PORT [-i INPUT] [-w TIMEOUT] [-h | "
         "--help]\n"
      << "\n"
      << "Arguments:\n"
      << "  -h, --help    Show help message\n"
      << "  -s            Server mode\n"
      << "  -c            Client mode\n"
      << "  -p PORT       UDP port number\n"
      << "  -a ADDRESS    Local bind address or destination host\n"
      << "  -i INPUT      Input file to send (client only, defaults to stdin)\n"
      << "  -o OUTPUT     Output file to write (server only, defaults to "
         "stdout)\n"
      << "  -w TIMEOUT    Timeout for protocol progress in seconds (default "
         "1)\n";
}

RdtConfig CliParser::parse(int argc, char *const *argv) {
  RdtConfig config;

  if (argc == 1) {
    print_help();
    throw std::invalid_argument("No arguments provided.");
  }

  // Reset getopt state for multiple calls (important for unit tests)
  optind = 1;
  opterr = 0;

  const char *const short_opts = "scnp:a:i:o:w:h";
  const option long_opts[] = {{"help", no_argument, nullptr, 'h'},
                              {nullptr, 0, nullptr, 0}};

  while (true) {
    const auto opt = getopt_long(argc, argv, short_opts, long_opts, nullptr);

    if (opt == -1)
      break;

    switch (opt) {
    case 's':
      config.is_server = true;
      break;
    case 'c':
      config.is_client = true;
      break;
    case 'p':
      try {
        config.port = std::stoi(optarg);
        if (config.port < 0 || config.port > 65535) {
          throw std::invalid_argument("Port must be between 0 and 65535.");
        }
      } catch (const std::exception &) {
        throw std::invalid_argument("Invalid port value: " +
                                    std::string(optarg));
      }
      break;
    case 'a':
      config.address = optarg;
      break;
    case 'i':
      config.input = optarg;
      break;
    case 'o':
      config.output = optarg;
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
      return config;
    case '?':
    default:
      throw std::invalid_argument("Invalid command line argument");
    }
  }

  // Validation
  if (config.is_server && config.is_client) {
    throw std::invalid_argument("Only one of -s or -c can be specified.");
  }
  if (!config.is_server && !config.is_client) {
    throw std::invalid_argument("Either -s or -c must be specified.");
  }
  if (config.port == -1) {
    throw std::invalid_argument("Port (-p) is mandatory.");
  }
  if (config.is_client && config.address.empty()) {
    throw std::invalid_argument("Host (-a) is mandatory for client mode.");
  }

  return config;
}

// ===============================================================
// UNIT TESTS
// ===============================================================

#ifndef DOCTEST_CONFIG_DISABLE

TEST_CASE("CLI Argument parsing - Server") {
  SUBCASE("Valid server minimal") {
    char *argv[] = {(char *)"./ipk-rdt", (char *)"-s", (char *)"-p",
                    (char *)"8080"};
    auto config = CliParser::parse(4, argv);
    CHECK(config.is_server == true);
    CHECK(config.is_client == false);
    CHECK(config.port == 8080);
    CHECK(config.address == "");
    CHECK(config.output == "-");
    CHECK(config.timeout == 1);
  }

  SUBCASE("Valid server full") {
    char *argv[] = {(char *)"./ipk-rdt", (char *)"-s",      (char *)"-p",
                    (char *)"1234",      (char *)"-a",      (char *)"127.0.0.1",
                    (char *)"-o",        (char *)"out.bin", (char *)"-w",
                    (char *)"5"};
    auto config = CliParser::parse(10, argv);
    CHECK(config.is_server == true);
    CHECK(config.port == 1234);
    CHECK(config.address == "127.0.0.1");
    CHECK(config.output == "out.bin");
    CHECK(config.timeout == 5);
  }

  SUBCASE("Missing port") {
    char *argv[] = {(char *)"./ipk-rdt", (char *)"-s"};
    CHECK_THROWS_AS(CliParser::parse(2, argv), const std::invalid_argument &);
  }
}

TEST_CASE("CLI Argument parsing - Client") {
  SUBCASE("Valid client minimal") {
    char *argv[] = {(char *)"./ipk-rdt", (char *)"-c", (char *)"-p",
                    (char *)"8080",      (char *)"-a", (char *)"localhost"};
    auto config = CliParser::parse(6, argv);
    CHECK(config.is_client == true);
    CHECK(config.is_server == false);
    CHECK(config.port == 8080);
    CHECK(config.address == "localhost");
    CHECK(config.input == "-");
    CHECK(config.timeout == 1);
  }

  SUBCASE("Valid client full") {
    char *argv[] = {(char *)"./ipk-rdt", (char *)"-c",     (char *)"-p",
                    (char *)"8080",      (char *)"-a",     (char *)"::1",
                    (char *)"-i",        (char *)"in.bin", (char *)"-w",
                    (char *)"10"};
    auto config = CliParser::parse(10, argv);
    CHECK(config.is_client == true);
    CHECK(config.port == 8080);
    CHECK(config.address == "::1");
    CHECK(config.input == "in.bin");
    CHECK(config.timeout == 10);
  }

  SUBCASE("Missing address") {
    char *argv[] = {(char *)"./ipk-rdt", (char *)"-c", (char *)"-p",
                    (char *)"8080"};
    CHECK_THROWS_AS(CliParser::parse(4, argv), const std::invalid_argument &);
  }
}

TEST_CASE("CLI Argument parsing - Common errors") {
  SUBCASE("Both -s and -c") {
    char *argv[] = {(char *)"./ipk-rdt", (char *)"-s", (char *)"-c",
                    (char *)"-p", (char *)"8080"};
    CHECK_THROWS_AS(CliParser::parse(5, argv), const std::invalid_argument &);
  }

  SUBCASE("Neither -s nor -c") {
    char *argv[] = {(char *)"./ipk-rdt", (char *)"-p", (char *)"8080"};
    CHECK_THROWS_AS(CliParser::parse(3, argv), const std::invalid_argument &);
  }

  SUBCASE("Invalid port number") {
    char *argv[] = {(char *)"./ipk-rdt", (char *)"-s", (char *)"-p",
                    (char *)"notaport"};
    CHECK_THROWS_AS(CliParser::parse(4, argv), const std::invalid_argument &);
  }

  SUBCASE("Invalid timeout value") {
    char *argv[] = {(char *)"./ipk-rdt", (char *)"-s", (char *)"-p",
                    (char *)"8080",      (char *)"-w", (char *)"-5"};
    CHECK_THROWS_AS(CliParser::parse(6, argv), const std::invalid_argument &);
  }
}

TEST_CASE("CLI Argument parsing - Help") {
  char *argv[] = {(char *)"./ipk-rdt", (char *)"--help"};
  auto config = CliParser::parse(2, argv);
  CHECK(config.help_requested == true);
}

#endif
