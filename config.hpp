#pragma once

#include <string>

struct Config {
	std::string editor;
};

Config load_config();
std::string get_config_dir();
std::string get_data_dir();
