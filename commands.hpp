#pragma once

#include "config.hpp"
#include "parser.hpp"

#include <string>

void usage(const char* prog);
std::string find_file(const std::string& hint);
int run_command(TaskFile& tf, const std::string& command, const Config& config, const std::string& filepath, const std::vector<std::string>& args = {});
