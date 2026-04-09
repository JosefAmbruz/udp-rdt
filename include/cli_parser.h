#pragma once

#include <cassert>
#include <cstdlib>
#include <getopt.h>
#include <iostream>
#include <stdexcept>
#include <stdlib.h>
#include <string>
#include <vector>

#include "dbg.h"
#include "doctest.h"

#define PARSING_COMPLETE opt == -1

// Taken from
// https://cfengine.com/blog/2021/optional-arguments-with-getopt-long/
#define OPTIONAL_ARGUMENT_IS_PRESENT                                           \
  ((optarg == NULL && optind < argc && argv[optind][0] != '-')                 \
       ? (bool)(optarg = argv[optind++])                                       \
       : (optarg != NULL))

struct ScannerConfig {
  std::string interface = "";
  int timeout = 1000;
  std::vector<std::string> subnets;
  bool help_requested = false;
  bool show_interfaces = false;
};

class CliParser {
public:
  static void print_help();

  static ScannerConfig parse(int argc, char *const *argv);
};
