#pragma once

#include <getopt.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

struct RdtConfig {
  bool is_server = false;
  bool is_client = false;
  std::string address = ""; // ADDRESS for server, HOST for client
  int port = -1;
  std::string input = "-";   // "-" means stdin
  std::string output = "-";  // "-" means stdout
  int timeout = 1;           // Default 1s
  bool help_requested = false;
};

class CliParser {
public:
  static void print_help();

  static RdtConfig parse(int argc, char *const *argv);
};
